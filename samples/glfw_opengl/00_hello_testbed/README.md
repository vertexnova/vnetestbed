# 00 - Hello Testbed

The baseline demo that every subsequent GLFW/OpenGL sample is built on. Proves the FBO → ImGui viewport pipeline and the Settings panel.

## What This Sample Shows

1. **FBO viewport**: Scene is rendered to a framebuffer that fills the ImGui "Viewport" window correctly.
2. **Viewport resize**: Resizing the viewport keeps the image filling the panel (no stretching).
3. **VSync**: Toggle in the application responds immediately.
4. **Grid and axes**: Spatial reference and confirmation that the camera and scene work.
5. **Orbit-arcball** (when `vne::interaction` is enabled): LMB drag rotates, RMB drag pans, scroll zooms.
6. **ImGui Settings**: Demo-specific section **Viewport** with controls reference and axis legend.

## Building

From the vnetestbed project root, enable samples and build:

```bash
cmake -B build -DVNE_TESTBED_SAMPLES=ON
cmake --build build
```

Or use a dev build (samples + tests):

```bash
cmake -B build -DVNE_TESTBED_DEV=ON
cmake --build build
```

**Using platform scripts** (see `scripts/README.md` for options):

```bash
# macOS
./scripts/build_macos.sh -t Debug -a configure_and_build

# Linux
./scripts/build_linux.sh -t Debug -a configure_and_build

# Windows (Git Bash / WSL)
./scripts/build_windows.sh -t Debug -a configure_and_build

# Windows (Visual Studio prompt)
python scripts/build_windows.py -t Debug -a configure_and_build
```

Scripts use build directories like `build/<BuildType>/build-macos-clang-*` or `build/<BuildType>/build-linux-*`. Configure with `-DVNE_TESTBED_SAMPLES=ON` if you need to enable samples when using scripts.

## Running

```bash
# After cmake -B build
./build/bin/samples/sample_00_hello_testbed
```

When using a script, run from that script’s build directory, e.g.:

```bash
./build/Debug/build-macos-clang-*/bin/samples/sample_00_hello_testbed
```

Exit with **ESC** or by closing the window.

## Key Concepts

- **BaseSceneLayer**: Renders grid, axes, and a perspective camera; shared with other samples via `samples/glfw_opengl/common/base_scene_layer.h`.
- **BaseInteractionLayer**: Orbit-arcball manipulator (optional); LMB orbit, RMB pan, scroll zoom.
- **ImGui viewport**: The scene is drawn into an ImGui window; multi-viewport layouts (2 or 4 panes) are supported when ImGui docking is enabled.
- **Libraries**: vne::testbed, vne::scene, vne::interaction (optional).

## Code Structure

- `demo_hello_testbed.cpp`: Registers the demo with `VNETESTBED_REGISTER_DEMO("hello_testbed", ...)` and pushes `BaseSceneLayer`, optional `BaseInteractionLayer`, and `HelloSettingsLayer` (Viewport panel).
- `../common/base_scene_layer.h`: Shared scene and interaction layer base used by 00, 01, 02, 03.
