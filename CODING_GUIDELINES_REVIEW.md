# CODING_GUIDELINES Compliance Review — vnetestbed

**Date:** February 2026  
**Scope:** vnetestbed library and samples/examples (`include/vertexnova/testbed/`, `src/vertexnova/testbed/`, `samples/`, `examples/`).

## Summary

- **Application harness and new code:** Aligned with CODING_GUIDELINES (naming, includes, Rule of Five, `#pragma once`, file layout). Enum values in **ApplicationDescriptor** were updated to use the required **`e` prefix** and **explicit values**.
- **Existing testbed code:** Mostly consistent; **enums** in `render_device.h`, `gl/` (e.g. `BlendFactor`, `ShaderDataType`, `SamplerWrap`, `GlfwGraphicsBackend`), and `window/glfw_window_descriptor.h` do **not** follow the guideline “enum values: `e` + PascalCase with explicit values” and are left as-is for a future cleanup.

---

## What Was Checked

| Guideline area | Status |
|----------------|--------|
| **C++ standard** | C++20 used as required. |
| **File names** | snake_case (e.g. `application_descriptor.h`, `demo_factory.h`, `logging_guard.cpp`). |
| **Header guards** | `#pragma once` in reviewed headers. |
| **Include order** | Project headers first, then system/third-party, with blank line between groups where applicable. |
| **Copyright / Author / Created** | Present in new/updated headers. |
| **Classes / interfaces** | PascalCase; interfaces use `I` prefix (e.g. `IWindow`, `ILayer`, `IPlugin`). |
| **Functions / methods** | camelCase (e.g. `runDemoApplication`, `swapBuffers`, `createDemo`). |
| **Private members** | snake_case + trailing `_` (e.g. `impl_`, `plugins_`, `layer_stack_`). |
| **Rule of Five / Zero** | Explicit delete or default where needed; e.g. `Application` move semantics, `ILayer` copy/move deleted. |
| **[[nodiscard]]** | Used where return value must be used (e.g. `isRunning()`, getters). |
| **noexcept** | Used on move constructors/assignments where appropriate. |
| **Macros** | ALL_CAPS (e.g. `VNETESTBED_REGISTER_DEMO`, `REGISTER_PLUGIN`). |
| **Enums (new)** | `WindowBackend` and `RenderBackend` in `application_descriptor.h` now use `e` prefix and explicit values. |

---

## Change Applied in This Review

- **`include/vertexnova/testbed/application_descriptor.h`**  
  - `WindowBackend`: `GLFW` → `eGLFW = 0`, `VneCross` → `eVneCross = 1`.  
  - `RenderBackend`: `OpenGL` → `eOpenGL = 0`.  
- **`src/vertexnova/testbed/application.cpp`**  
  - All references to these enums updated to the new enumerators (`eGLFW`, `eOpenGL`).

---

## Remaining Non-Compliance (Optional Future Cleanup)

- **Enum naming and explicit values** (guideline: “enum values: `e` + PascalCase + explicit value”):  
  The following enums do **not** follow this yet; updating them would require a project-wide pass and call-site updates:  
  - `render_device.h`: `BlendFactor`, `BlendEquation`, `CompareFunc`, `CullMode`, `DrawMode`, `TextureFormat`  
  - `gl/buffer_layout.h`: `ShaderDataType`  
  - `gl/sampler2d.h`: `SamplerWrap`, `SamplerFilter`  
  - `gl/texture2d.h`: `Texture2DFormat`  
  - `gl/draw.h`: `DrawMode`  
  - `window/glfw_window_descriptor.h`: `GlfwGraphicsBackend` (e.g. `OpenGL`, `OpenGLES` → `eOpenGL`, `eOpenGLES` with explicit values)

No other systematic violations were found in the reviewed application/samples harness code.
