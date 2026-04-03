#==============================================================================
# VNEPrivateDeps.cmake - Optional private dependency vneinteraction
#
# Prerequisites: VneTestbedDeps included; VNE_DEPS_INTERNAL_DIR set.
# When VNE_WITH_VNEINTRACTION=ON (default in vnetestbed) we add vne::interaction; when OFF we
# never add_subdirectory or require the dir. Submodule tests stay OFF unless you change cache.
#==============================================================================

if(VNE_WITH_VNEINTRACTION)
  set(VNEINTRACTION_DIR "${VNE_DEPS_INTERNAL_DIR}/vneinteraction")
  if(EXISTS "${VNEINTRACTION_DIR}/CMakeLists.txt")
    # vneinteraction uses VNE_INTERACTION_TESTS (not BUILD_TESTS). Keep its gtest suite out of vnetestbed;
    # run tests from a standalone vneinteraction build or enable explicitly via cache if needed.
    vnetestbed_use_dep(TARGET vne::interaction SUBDIR vneinteraction DEPS_DIR INTERNAL
      CACHE_VARS
        BUILD_TESTS OFF
        BUILD_EXAMPLES OFF
        VNE_INTERACTION_TESTS OFF
        VNE_INTERACTION_DEV OFF
        VNE_INTERACTION_EXAMPLES OFF
      SAVE_RESTORE
        BUILD_TESTS
        BUILD_EXAMPLES
        VNE_INTERACTION_TESTS
        VNE_INTERACTION_DEV
        VNE_INTERACTION_EXAMPLES)
    message(STATUS "vneinteraction enabled (from submodule).")
  else()
    message(FATAL_ERROR
      "VNE_WITH_VNEINTRACTION=ON but submodule 'deps/internal/vneinteraction' is missing.\n"
      "Run: git submodule update --init deps/internal/vneinteraction\n"
      "Or disable: -DVNE_WITH_VNEINTRACTION=OFF")
  endif()
else()
  message(STATUS "vneinteraction disabled. Building without private dependency.")
endif()
