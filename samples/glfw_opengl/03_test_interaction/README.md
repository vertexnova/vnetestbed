# 03 - Test Interaction

Full **interaction** demo on top of [`BaseSceneLayer`](../common/base_scene_layer.h): every high-level controller type (Inspect orbit or virtual trackball with Hyperbolic/Rim projection, 3D navigation with FPS/Fly/Game modes, orthographic pan/zoom, follow), zoom methods, view-direction presets, and optional mesh loading from testdata when `VNE_TESTBED_VNEIO` is enabled. It shares the same **build / run / IDE** workflow as [`00_hello_testbed`](../00_hello_testbed/README.md) and uses the same Blinn-Phong [`MeshLayer`](../../../include/vertexnova/testbed/utils/mesh_layer.h) path as the scene samples when meshes are available.

Screenshots are not included yet; they may be added in a later revision.

## What This Sample Shows

1. **Base scene**: Grid, axes, perspective or orthographic camera from `BaseSceneLayer`; settings can toggle grid/axes and switch projection.
2. **Controllers**: Runtime selection among Inspect (Euler orbit or trackball), Navigation (FPS / Fly / Game via `NavigateMode`), Ortho2D, and Follow; per-controller ImGui tuning (trackball projection when in trackball inspect mode, pivot mode, speeds, navigation multipliers, ortho/follow options).
3. **Zoom**: Dolly-to-COI, scene-scale, or FOV change where applicable; matrix readouts for view and projection when enabled.
4. **Mesh browser** (with vneio): Lists mesh files under the configured directory (PLY, OBJ, STL, FBX, glTF), loads on click, drag-and-drop to the viewport, plus lighting and mesh transform panels when a mesh is loaded.
5. **Layer stack**: `BaseSceneLayer` → `InteractionTestLayer` → optional `MeshLayer` → `InteractionSettingsLayer` (ImGui settings).

## Try it (quick checklist)

1. Run `03_test_interaction`.
2. Use **Settings** to pick camera type, controller type, zoom method, and (for Inspect) view-direction presets; try **Reset camera** under Camera State.
3. With vneio enabled, open **Mesh Browser** and load a mesh or drag a listed file onto a viewport.
4. Exit with **ESC** or by closing the window.

## Building

**Preferred: use the project scripts** (they pick a compiler, build layout, and optional presets). See [`scripts/README.md`](../../../scripts/README.md) for full options.

```bash
# macOS
./scripts/build_macos.sh -t Debug -a configure_and_build

# Linux
./scripts/build_linux.sh -t Debug -a configure_and_build

# Windows (Git Bash / WSL)
./scripts/build_windows.sh -t Debug -a configure_and_build

# Windows (Visual Studio Developer Command Prompt — recommended on Windows)
python scripts/build_windows.py -t Debug -a configure_and_build
```

Scripts place outputs under directories such as `build/<BuildType>/build-macos-clang-*` or `build/<BuildType>/build-linux-*`. Enable samples with CMake if your configure step does not already turn them on, for example `-DVNE_TESTBED_SAMPLES=ON` or `-DVNE_TESTBED_DEV=ON` (samples + tests). This target also requires `vne::interaction` (see top-level CMake options).

**Manual CMake** (from the vnetestbed repository root):

```bash
cmake -B build -DVNE_TESTBED_SAMPLES=ON
cmake --build build
```

### Using an IDE

Open the same CMake project and build the **`03_test_interaction`** target; run the executable from `bin/samples` under your build tree.

## Running

```bash
# After a plain cmake -B build
./build/bin/samples/03_test_interaction
```

When you built with a script, run the executable under that script’s build directory, for example:

```bash
./build/Debug/build-macos-clang-*/bin/samples/03_test_interaction
```

`VNETESTBED_TESTDATA_DIR` is set by CMake to the repository `testdata` root so mesh paths resolve when vneio is enabled.

## Key Concepts

- **`ControllerVariant`**: `std::variant` over Inspect, Navigation3D, Ortho2D, and Follow controllers; dispatch uses `std::visit` for resize, events, and updates.
- **`InteractionTestLayer`**: One controller instance per viewport (up to four), synced with scene cameras from `BaseSceneLayer::getActiveCameras()`.
- **`InteractionSettingsLayer`**: ImGui panel; UI state is grouped in `InteractionUiSettings` with `uiSettings()` accessors (same idea as [`SceneUiSettings`](../02_test_scene/demo_test_scene.h) in `02_test_scene`).
- **Optional vneio**: Mesh listing, reload, drag-and-drop, and Phong lighting UI require `VNE_TESTBED_VNEIO` and the vneio-linked `MeshLayer`.

## Code Structure

- [`demo_test_interaction.h`](demo_test_interaction.h) / [`demo_test_interaction.cpp`](demo_test_interaction.cpp): `InteractionTestLayer`, `InteractionSettingsLayer`, `registerTestInteractionDemo` (`VNETESTBED_REGISTER_DEMO("test_interaction", ...)`).
- [`../common/base_scene_layer.h`](../common/base_scene_layer.h): Shared `BaseSceneLayer` and optional `BaseInteractionLayer` (not used in this demo; interaction is driven by `InteractionTestLayer` directly).

For a heavier **scene** sample (lights, multiple cubes, no mesh IO), see [`02_test_scene`](../02_test_scene/README.md).
