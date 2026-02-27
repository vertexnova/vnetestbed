# Testbed plugin system: design and history

This document has two parts:

1. **Implemented architecture** — The codebase uses a **layer-based** design: **IPlugin** is a factory (`getName()`, `createLayers()`); **ILayer** has the lifecycle hooks; **LayerStack** drives layers. This is the intended, current design. It is described in **testbed.md** and summarized below.
2. **Alternative design (not implemented)** — Sections 1–7 below describe a different design (PluginManager driving IPlugin lifecycle with onInit/onUpdate/onRender/onImGui/onShutdown). That design was considered but **not** adopted. It is kept here for context and possible future reference only.

---

## Implemented architecture (layer-based)

**Authoritative reference:** [testbed.md](testbed.md).

| Concept | In this codebase |
|--------|-------------------|
| **IPlugin** | Factory only: `getName()`, `createLayers()` → `vector<unique_ptr<ILayer>>`. No lifecycle methods. |
| **ILayer** | Runtime unit with lifecycle: `onAttach(AppContext&)`, `onDetach`, `onUpdate(float dt)`, `onBeginRender`, `onRender`, `onGuiBegin`, `onGuiRender`, `onGuiEnd`, `onEvent`. |
| **Driver** | **LayerStack** (not PluginManager). Runner calls `stack.onUpdate(dt)`, `stack.onRender(ctx)`, etc. |
| **Registration** | **PluginRegistry** singleton + `REGISTER_PLUGIN(PluginClass)`; `createAndPushLayers(stack, ctx)` pushes all plugins’ layers onto the stack. |
| **Context** | **AppContext** (IWindow*, IRenderAdapter*, IDebugDraw*) passed to layers at **onAttach** only. Per-frame: **RenderContext** (FrameInfo, debug_draw) passed to layer callbacks. |
| **IRenderAdapter** | init(void*), beginFrame(), endFrame(), shutdown(). |

Plugins do not receive lifecycle callbacks. Layers do. The runner populates a LayerStack (via PluginRegistry or manual pushLayer), then drives the stack each frame.

---

## Alternative design (not implemented): PluginManager + IPlugin lifecycle

The following sections describe a design where **IPlugin** has onInit/onUpdate/onRender/onImGui/onShutdown and a **PluginManager** drives plugins. **This is not what the codebase implements.** It is retained for design history and possible future evolution.

---

### 1. Existing vs new (summary)

| Aspect | Existing (vnedevtestbed) | New (pasted design) |
|--------|---------------------------|----------------------|
| **Plugin lifecycle** | Every hook takes `AppContext&` and (where needed) `double deltaTime` | Hooks take no context; `onUpdate(float dt)` only for dt |
| **Context** | `AppContext` holds `IWindow*`, `IRendererAdapter*`, `IDebugDraw*`; passed every frame | No AppContext in API; “plugins receive adapter when they need it” |
| **Registration** | `PluginRegistry` singleton + `REGISTER_PLUGIN(PluginClass)` at static init | `PluginManager::addPlugin(unique_ptr<IPlugin>)`; explicit only |
| **Driver** | Runner gets `getPlugins()`, calls init/update/render/gui/shutdown in order | `PluginManager`: `init()`, `update(dt)`, `render()`, `imGui()`, `shutdown()` |
| **Window** | `IWindow`: getWidth, getHeight, pollEvents, shouldClose | (Not in pasted snippet; keep existing) |
| **Renderer** | `IRendererAdapter`: beginFrame, endFrame only | `IRenderAdapter`: init(window_handle), beginFrame, endFrame, shutdown |
| **Debug draw** | `IDebugDraw`: single `draw()` | `IDebugDraw`: line, aabb, text, flush; Vec3/DebugAabb |
| **ImGui phase** | `onGui(ctx)` | `onImGui()` |
| **Delta time** | `double deltaTime` | `float dt` |

---

### 2. Design decisions (combined)

- **Single plugin interface**: One `IPlugin` type used by both the runner and unit tests.
- **Context at init only**: Plugins receive `AppContext&` only in `onInit(AppContext& ctx)`. They may store `ctx.window`, `ctx.renderer`, `ctx.debugDraw` for use in later hooks. All other hooks are context-free: `onUpdate(float dt)`, `onRender()`, `onImGui()`, `onShutdown()`. This keeps tests simple (mock AppContext at init or empty struct) and matches the “lifecycle-only” style of the new design.
- **PluginManager as the driver**: The runner uses a `PluginManager` that owns plugins and drives `init(ctx)`, `update(dt)`, `render()`, `imGui()`, `shutdown()` in the documented order. Shutdown runs in reverse registration order.
- **Optional static registration**: Keep `PluginRegistry` + `REGISTER_PLUGIN(PluginClass)` as an optional convenience. The runner can either (a) add plugins explicitly to a `PluginManager` or (b) pull from `PluginRegistry::instance().getPlugins()` and then add them to the manager (or the manager can accept a registry in a future overload). So: PluginManager is the single driver; Registry is optional for discovery.
- **AppContext and backend interfaces**: Keep `AppContext` with `IWindow*`, `IRenderAdapter*`, `IDebugDraw*`. Rename `IRendererAdapter` → `IRenderAdapter` and extend with `init(void* window_handle)` and `shutdown()` so the render adapter owns its lifecycle. Keep `IWindow` as-is. Upgrade `IDebugDraw` to the rich API with `vne::math::Vec3f`.
- **Naming and layout**: Follow CODING_GUIDELINES.md: file names snake_case, no `i_` prefix; interface classes use `I` + PascalCase. Headers: `plugin.h`, `app_context.h`, `window.h`, `render_adapter.h`, `debug_draw.h`, `plugin_manager.h`, `plugin_registry.h`, and `plugins/scene_inspector_plugin.h`. Namespace: `vne::testbed`. All under `include/vertexnova/testbed/` and `src/vertexnova/testbed/`.

---

### 3. Final API (concise)

#### 3.1 Plugin lifecycle (`plugin.h`)

```cpp
namespace vne::testbed {

struct AppContext;  // forward

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual void onInit(AppContext& ctx) = 0;
    virtual void onUpdate(float dt) = 0;
    virtual void onRender() = 0;
    virtual void onImGui() = 0;
    virtual void onShutdown() = 0;
};

}
```

- **onInit(AppContext& ctx)** called once; plugins may store pointers from `ctx` (e.g. window, renderer, debugDraw).
- **onUpdate(dt)**, **onRender()**, **onImGui()**, **onShutdown()** take no context; use `float` for dt to match the new design and tests.

#### 3.2 AppContext and backend interfaces

- **AppContext** (`app_context.h`): struct holding `IWindow* window`, `IRenderAdapter* renderer`, `IDebugDraw* debugDraw` (all nullable). Runner fills it once and passes it to `PluginManager::init(ctx)`.
- **IWindow** (`window.h` or in `app_context.h`): getWidth(), getHeight(), pollEvents(), shouldClose(). Unchanged from existing.
- **IRenderAdapter** (`render_adapter.h`): init(void* window_handle), beginFrame(), endFrame(), shutdown(). Replaces the old “IRendererAdapter” with only begin/end; add init/shutdown so the adapter owns its lifecycle.
- **IDebugDraw** (`debug_draw.h`): Use `vne::math::Vec3f` and a struct `DebugAabb { Vec3f min; Vec3f max; }`. Methods: line(from, to, color), aabb(box, color), text(pos, label), flush(). Replaces the single draw().

#### 3.3 Plugin manager and optional registry

- **PluginManager** (`plugin_manager.h` / `.cpp`):
  - addPlugin(std::unique_ptr<IPlugin>)
  - init(AppContext& ctx)  // calls onInit(ctx) on each plugin in order
  - update(float dt)
  - render()
  - imGui()
  - shutdown()  // calls onShutdown() in reverse order, then clears list
  - pluginCount() const

- **PluginRegistry** (optional, `plugin_registry.h` / `.cpp`): singleton, registerPlugin(name, unique_ptr<IPlugin>), getPlugins(). Macro REGISTER_PLUGIN(PluginClass) for static registration. Runner may use it to discover plugins and then add them to a PluginManager, or add plugins explicitly.

#### 3.4 Example plugin

- **SceneInspectorPlugin** (`plugins/scene_inspector_plugin.h`): implements `IPlugin` with empty overrides; stub for a future scene-graph inspector UI.

---

### 4. File layout (final)

```
include/vertexnova/testbed/
  plugin.h              IPlugin (onInit(AppContext&), onUpdate(dt), onRender, onImGui, onShutdown)
  app_context.h         AppContext struct; forward decl or includes for IWindow, IRenderAdapter, IDebugDraw
  window.h              IWindow
  render_adapter.h      IRenderAdapter (init, beginFrame, endFrame, shutdown)
  debug_draw.h          IDebugDraw (Vec3f, DebugAabb, line, aabb, text, flush); uses vne::math::Vec3f
  plugin_manager.h      PluginManager
  plugin_registry.h     PluginRegistry + REGISTER_PLUGIN (optional)
  plugins/
    scene_inspector_plugin.h

src/vertexnova/testbed/
  plugin_manager.cpp
  plugin_registry.cpp   (if keeping registry)
```

All headers listed in `src/CMakeLists.txt` (e.g. HEADER_FILES / source_group) and built as part of the vnetestbed static library.

---

### 5. Runner flow (examples/02_plugin_runner or equivalent)

1. Create window (e.g. GLFW), create render adapter (e.g. OpenGL), optionally create debug-draw implementation.
2. Build AppContext: set window, renderer, debugDraw.
3. Create PluginManager; add plugins (explicit addPlugin and/or from PluginRegistry::getPlugins()).
4. pluginManager.init(ctx).
5. Main loop while !window->shouldClose():  
   pluginManager.update(dt);  
   renderer->beginFrame();  
   pluginManager.render();  
   pluginManager.imGui();  
   renderer->endFrame();  
   swap buffers; window->pollEvents();
6. pluginManager.shutdown().
7. renderer->shutdown(); destroy window; etc.

Existing vnedevtestbed runner logic (order of init/update/render/gui/shutdown, reverse shutdown) is preserved; the only API change is IPlugin taking AppContext only in onInit and using float dt in onUpdate.

---

### 6. Unit tests

- **PluginManager**: StartsEmpty, AddPluginIncreasesCount, InitCallsOnInitOnAllPlugins (pass a mock or empty AppContext), UpdatePassesDeltaTime, RenderCallsOnRenderOnAllPlugins, ImGuiCallsOnImGuiOnAllPlugins, ShutdownCallsOnShutdownInReverseOrder, ShutdownClearsPlugins.
- **SceneInspectorPlugin**: CanBeInstantiatedAndRegistered; run full lifecycle (init with empty AppContext, update, render, imGui, shutdown) without crash.
- **Interface sanity**: static_assert that IPlugin, IRenderAdapter, IDebugDraw are abstract and that a concrete type (e.g. RecordingPlugin) is a non-abstract derived class of IPlugin.

Use Google Test; optionally VNE_LOG_* in one test to confirm logging; follow CODING_GUIDELINES.md.

---

### 7. Migration from current vnedevtestbed

- Move headers/sources into `include/vertexnova/testbed/` and `src/vertexnova/testbed/`; namespace `vne::devtestbed` → `vne::testbed`; includes `vertexnova/devtestbed/` → `vertexnova/testbed/`.
- Change IPlugin to the combined signature: onInit(AppContext&) once; onUpdate(float), onRender(), onImGui(), onShutdown() with no context.
- Rename IRendererAdapter → IRenderAdapter; add init(void*), shutdown() in the interface (implementations in runner).
- Replace IDebugDraw::draw() with line/aabb/text/flush and Vec3f/DebugAabb in debug_draw.h.
- Add PluginManager; implement init(ctx), update(dt), render(), imGui(), shutdown(); runner uses PluginManager and optionally fills it from PluginRegistry.
- Keep PluginRegistry + REGISTER_PLUGIN for backward compatibility and convenience.
- Runner: build AppContext, create PluginManager, add plugins, then init(ctx) then loop then shutdown(); use float for dt in the loop.

This yields one consistent design: context at init, lifecycle-only for the rest, PluginManager as driver, optional Registry, richer IDebugDraw and IRenderAdapter, and a single namespace and file set under vertexnova/testbed.
