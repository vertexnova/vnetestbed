# Dependencies

This directory holds external and internal dependencies for VneTemplate.

## Layout

- **external/** – Third-party dependencies (e.g. Google Test).
- **internal/** – VertexNova internal libraries (vnecommon, vnelogging).

CMake modules (vnecmake) live in **cmake/vnecmake** at the project root; see main [README](../README.md).

## Getting dependencies

### vnecmake (required)

CMake modules come from the **vnecmake** submodule at `cmake/vnecmake`. From the project root:

```bash
git submodule add <vnecmake-repo-url> cmake/vnecmake
git submodule update --init --recursive
```

If already in `.gitmodules`, run `git submodule update --init --recursive`.

### External: Google Test

Tests use Google Test from `deps/external/googletest`. Either:

1. **Git submodule (recommended)**  
   From the project root:
   ```bash
   git submodule add https://github.com/google/googletest.git deps/external/googletest
   git submodule update --init --recursive
   ```
   Use tag `v1.17.0` (or later) if you pin:  
   `cd deps/external/googletest && git checkout v1.17.0`

2. **FetchContent fallback**  
   If `deps/external/googletest` is not present, the CMake configuration for tests will use FetchContent to download googletest (v1.17.0) at configure time.

### Internal: vnecommon and vnelogging

The library optionally links to VertexNova internal dependencies when present:

- **deps/internal/vnecommon** – Common utilities.
- **deps/internal/vnelogging** – Logging (e.g. spdlog-based).

These are not shipped with the template. Add them as git submodules or clone the repositories into the paths above. See the README in each internal dependency directory for instructions and recommended versions.

After adding internal deps, run:

```bash
git submodule update --init --recursive
```

(or your project’s equivalent) so that all submodules are available.
