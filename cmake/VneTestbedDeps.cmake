#==============================================================================
# VneTestbedDeps.cmake - Helper to add vnetestbed dependencies with save/restore
#
# Prerequisites: VneUseDep included; VNE_DEPS_INTERNAL_DIR, VNE_DEPS_EXTERNAL_DIR,
# CMAKE_BINARY_DIR set.
#
# Usage:
#   vnetestbed_use_dep(TARGET vne::common SUBDIR vnecommon DEPS_DIR INTERNAL
#     CACHE_VARS BUILD_EXAMPLES OFF SAVE_RESTORE BUILD_EXAMPLES)
#   vnetestbed_use_dep(TARGET glad::glad SUBDIR glad DEPS_DIR INTERNAL)
#==============================================================================

function(vnetestbed_use_dep)
    set(oneValueArgs TARGET SUBDIR DEPS_DIR)
    set(multiValueArgs CACHE_VARS SAVE_RESTORE)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "vnetestbed_use_dep: TARGET is required")
    endif()
    if(NOT ARG_SUBDIR)
        message(FATAL_ERROR "vnetestbed_use_dep: SUBDIR is required")
    endif()
    if(NOT ARG_DEPS_DIR)
        message(FATAL_ERROR "vnetestbed_use_dep: DEPS_DIR is required (INTERNAL or EXTERNAL)")
    endif()

    if(ARG_DEPS_DIR STREQUAL "INTERNAL")
        set(_subdir_path "${VNE_DEPS_INTERNAL_DIR}/${ARG_SUBDIR}")
        set(_binary_dir "${CMAKE_BINARY_DIR}/deps/internal/${ARG_SUBDIR}")
    elseif(ARG_DEPS_DIR STREQUAL "EXTERNAL")
        set(_subdir_path "${VNE_DEPS_EXTERNAL_DIR}/${ARG_SUBDIR}")
        set(_binary_dir "${CMAKE_BINARY_DIR}/deps/external/${ARG_SUBDIR}")
    else()
        message(FATAL_ERROR "vnetestbed_use_dep: DEPS_DIR must be INTERNAL or EXTERNAL, got ${ARG_DEPS_DIR}")
    endif()

    # Save CACHE vars that should be restored after (so parent options are preserved)
    foreach(_var IN LISTS ARG_SAVE_RESTORE)
        get_property(_ty CACHE ${_var} PROPERTY TYPE)
        if(NOT _ty STREQUAL "NOTFOUND" AND _ty)
            set(_vtd_saved_${_var} "${${_var}}")
        endif()
    endforeach()

    # For vnelogging-style deps: force BUILD_TESTS/BUILD_EXAMPLES OFF before add_subdirectory
    if(ARG_SAVE_RESTORE)
        foreach(_var IN LISTS ARG_SAVE_RESTORE)
            if(_var STREQUAL "BUILD_TESTS" OR _var STREQUAL "BUILD_EXAMPLES")
                set(${_var} OFF CACHE BOOL "" FORCE)
            endif()
        endforeach()
    endif()

    # Add the dependency (VneUseDep applies CACHE_VARS and add_subdirectory)
    if(ARG_CACHE_VARS)
        vne_use_dep(TARGET ${ARG_TARGET} SUBDIR "${_subdir_path}" BINARY_DIR "${_binary_dir}"
            CACHE_VARS ${ARG_CACHE_VARS})
    else()
        vne_use_dep(TARGET ${ARG_TARGET} SUBDIR "${_subdir_path}" BINARY_DIR "${_binary_dir}")
    endif()

    # Restore saved CACHE vars
    foreach(_var IN LISTS ARG_SAVE_RESTORE)
        if(DEFINED _vtd_saved_${_var})
            set(${_var} "${_vtd_saved_${_var}}" CACHE BOOL "" FORCE)
        else()
            unset(${_var} CACHE)
        endif()
    endforeach()
endfunction()
