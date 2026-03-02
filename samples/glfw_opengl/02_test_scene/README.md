# 02 - Test Scene

Everything from **01_test_events** (grid, axes, orbit camera, Events panel) plus a full **Scene** demo: perspective/ortho camera, rotating cubes, and configurable lighting (ambient, directional, spot, point lights). Use this sample to exercise vnescene cameras/lights, IRenderDevice indexed drawing, and Blinn-Phong GLSL. **Shaders are fully in files** for both OpenGL (desktop) and OpenGL ES; the correct pair is chosen at runtime by backend.

## What This Sample Shows

1. **Camera**: Switch at runtime between `PerspectiveCamera` and `OrthographicCamera`; FOV, near/far (perspective) or half-extent (ortho); position, target, up. Optional debug visuals (position cross, target cross, up vector, view direction).
2. **Cubes**: 3–4 rotating cubes drawn with `IRenderDevice::drawIndexed`; per-cube model matrix display in the Settings panel.
3. **Lighting**: Ambient and directional lights; one spot light (position, direction, inner/outer angles, range); up to 4 point lights (add/remove, orbit radius/speed). Optional attenuation formula (constant + linear×d + quadratic×d²) for point/spot.
4. **Shaders**: All shader code lives in `shaders/`. For **OpenGL** (desktop) the sample loads `scene_vert.glsl` and `scene_frag.glsl` (#version 410 core). For **OpenGL ES** it loads `scene_vert_es.glsl` and `scene_frag_es.glsl` (#version 300 es). Files are copied next to the executable by the build; if they are missing, the scene will not draw and an error is logged.
5. **Interaction**: Toggle VNE camera interaction (orbit/pan/zoom) on or off; reset scene to default (camera, cubes, lights, options).

## ImGui Settings Panel Sections

- **[Camera]**: Type (Perspective / Orthographic), Position, Target, Up; FOV / Near / Far (perspective) or Half-extent (ortho); Show camera visuals, Show view matrix, Show projection matrix.
- **[Interaction]**: Enable/disable VNE camera interaction; Reset scene to default.
- **[Cubes]**: Count (3–4), Rotation speed; per-cube collapsing section with model matrix.
- **[Ambient]**: Enabled, Color, Intensity.
- **[Directional Light]**: Enabled, Direction, Color, Intensity.
- **[Point Lights]**: Add/remove (max 4); per-light Enabled, Color, Intensity, Range, Orbit radius/speed.
- **[Spot Light]**: Enabled, Position, Direction, Color, Intensity, Range, Inner/Outer angle (deg). Angles are validated (outer > inner + ε) to avoid NaNs in the shader.
- **[Attenuation]**: Use formula toggle; Constant, Linear, Quadratic (when enabled).

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

Shaders are copied to the executable directory by a POST_BUILD step so they can be loaded at runtime when running from the build tree.

## Running

```bash
# After cmake -B build
./build/bin/samples/sample_02_test_scene
```

When using a script, run from that script's build directory, e.g.:

```bash
./build/Debug/build-macos-clang-*/bin/samples/sample_02_test_scene
```

Exit with **ESC** or by closing the window. Use the Settings panel to change camera, cubes, and lights; enable the spot light and point lights to see their effect.

## Key Concepts

- **PerspectiveCamera / OrthographicCamera**: vnescene cameras with view and projection matrices; aspect ratio updated on viewport resize.
- **SceneState**: Holds ambient, directional, spot, and point lights; cleared and repopulated each frame from ImGui state.
- **Spot light angles**: Inner/outer angles are clamped so outer > inner + ε (CPU and shader epsilon guard) to keep the spot falloff denominator non-zero and avoid NaNs.
- **Shaders from files**: Shader code is only in files (no embedded fallback). Desktop build uses `scene_vert.glsl` / `scene_frag.glsl`; OpenGL ES build uses `scene_vert_es.glsl` / `scene_frag_es.glsl`. Paths are resolved from cwd or `bin/samples`.
- **Libraries**: vne::testbed, vne::scene, vne::events, vne::interaction (optional).

## Code Structure

- `demo_test_scene.cpp`: Registers the demo, pushes `BaseSceneLayer` (with camera, cubes, lights, and scene shader), optional `BaseInteractionLayer`, `EventsLayer`, and `TestSceneSettingsLayer` (Camera, Interaction, Cubes, Ambient, Directional Light, Point Lights, Spot Light, Attenuation panels).
- `shaders/scene_vert.glsl`, `shaders/scene_frag.glsl`: OpenGL (410 core) vertex and fragment shaders.
- `shaders/scene_vert_es.glsl`, `shaders/scene_frag_es.glsl`: OpenGL ES (300 es) vertex and fragment shaders (same lighting, with `precision mediump float` and ES-compatible syntax).
- `../common/base_scene_layer.h`: Shared scene and interaction layer base used by 00, 01, 02, 03.
