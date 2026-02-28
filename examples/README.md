# VneTestbed Examples

This directory contains examples demonstrating the VneTestbed API.

## Building Examples

From the project root:

```bash
cmake -B build -DVNE_TESTBED_CI=OFF -DVNE_TESTBED_DEV=ON
cmake --build build
```

Alternatively, `-DVNE_TESTBED_DEV=ON` enables both tests and examples (when not in CI mode).

Executables are placed in `build/bin/examples/`.

## Available Examples

### 01_hello_testbed — Minimal bootstrap

Minimal program: initializes logging and prints a message. Useful to verify the build and logging setup.

**Run:** `./build/bin/examples/example_01_hello_testbed`

### 02_plugin_runner — Layer-based runner

Full testbed runner: creates a window (GLFW), OpenGL context, and a **LayerStack**; uses **PluginRegistry::createAndPushLayers** to add layers from registered plugins (e.g. SceneInspector); drives the layer lifecycle each frame (onUpdate, onBeginRender, onRender, onGuiBegin/GuiRender/GuiEnd). Demonstrates **AppContext**, **LayerStack**, **IPlugin**/ **ILayer**, and **IRenderAdapter**.

**Run:** `./build/bin/examples/example_02_plugin_runner`

## Quick Reference

| Example           | Focus              | Key concepts                                      |
|-------------------|--------------------|---------------------------------------------------|
| 01_hello_testbed  | Minimal bootstrap  | Logging guard, build verification                 |
| 02_plugin_runner  | Layer/plugin runner| AppContext, LayerStack, IPlugin, ILayer, IRenderAdapter |

For the full API, see [docs/vertexnova/testbed/testbed.md](../docs/vertexnova/testbed/testbed.md).
