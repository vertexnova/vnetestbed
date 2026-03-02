# 01 - Test Events

Everything from **00_hello_testbed** (grid, axes, orbit camera) plus a live **Events** panel in the Settings sidebar. Use this sample to verify that every vneevents type is delivered correctly and to exercise input polling.

## What This Sample Shows

1. **Event delivery**: Keyboard, mouse button, mouse move, scroll, window resize, window close, and touch (press/move/release) events arrive with correct type and data.
2. **Touch emulation**: Left mouse button is emulated as touch id `0` (TouchPress, TouchMove, TouchRelease). Only enabled in this demo via `GlfwWindow::setTouchEmulationEnabled(true)` so you can exercise touch on desktop.
3. **Event ordering**: Move vs pressed vs released ordering and event log in the UI.
4. **EventManager queue**: Events/sec counter and total events received (Stats section).
5. **Input polling**: Live key-held state for WASD, Space, Escape; current mouse position and scroll; Keyboard, Mouse, Char entry, and Touch sections. Shows held state separately from discrete events.

## ImGui Settings Panel Sections

- **[Events]**: Last N events (key, mouse, window resize/close, touch) with type, data, and frame.
- **[Input Poll]**: Live state for WASD + Space + Escape, mouse position, scroll; Keyboard, Mouse, Char entry, Touch.
- **[Stats]**: Total events received, events this second.

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
./build/bin/samples/sample_01_test_events
```

When using a script, run from that script’s build directory, e.g.:

```bash
./build/Debug/build-macos-clang-*/bin/samples/sample_01_test_events
```

Interact with the window (keys, mouse, scroll, resize) and watch the Events and Input Poll panels. Exit with **ESC** or by closing the window.

## Key Concepts

- **EventsLayer**: Implements `onEvent()` and records events for the Events panel; exposes stats for the UI.
- **Touch emulation**: Opt-in per window via `GlfwWindow::setTouchEmulationEnabled(true)`; only this sample enables it so other samples do not get duplicate mouse + touch.
- **Input polling**: Uses `vne::events::Input::mousePosition()`, `Input::isKeyPressed()`, etc., to show current state alongside the event stream.
- **Libraries**: vne::testbed, vne::scene, vne::events, vne::interaction (optional).

## Code Structure

- `demo_test_events.cpp`: Registers the demo, enables touch emulation on the GLFW window, then pushes `BaseSceneLayer`, optional `BaseInteractionLayer`, `EventsLayer`, and `EventsSettingsLayer` (Events / Input Poll / Stats panels).
- `../common/base_scene_layer.h`: Shared scene and interaction layer base.
