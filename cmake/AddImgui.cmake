#==============================================================================
# AddImgui.cmake - Build Dear ImGui from submodule (docking branch)
#
# Creates the imgui target when the imgui submodule is present.
# Requires: glfw, glad::glad, VNE_DEPS_EXTERNAL_DIR, VNE_TARGET_PLATFORM
#==============================================================================

set(_imgui_dir "${VNE_DEPS_EXTERNAL_DIR}/imgui")
if(NOT EXISTS "${_imgui_dir}/imgui.h")
    message(FATAL_ERROR "AddImgui: imgui submodule not found at ${_imgui_dir}. Run: git submodule update --init deps/external/imgui")
endif()

set(_imgui_sources
    ${_imgui_dir}/imgui.cpp
    ${_imgui_dir}/imgui_draw.cpp
    ${_imgui_dir}/imgui_tables.cpp
    ${_imgui_dir}/imgui_widgets.cpp
    ${_imgui_dir}/imgui_demo.cpp
    ${_imgui_dir}/backends/imgui_impl_glfw.cpp
    ${_imgui_dir}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${_imgui_sources})
target_include_directories(imgui
    PUBLIC
        $<BUILD_INTERFACE:${_imgui_dir}>
        $<BUILD_INTERFACE:${_imgui_dir}/backends>
        $<INSTALL_INTERFACE:include>
)

# OpenGL ES: imgui_impl_opengl3 needs IMGUI_IMPL_OPENGL_ES3 when using ES backend.
# Key on VNE_TESTBED_OPENGLES (active backend) so Web/visionOS ES builds are correct.
# Fallback to platform check when option not yet resolved (AddImgui runs before src/).
if(VNE_TESTBED_OPENGLES OR
   VNE_TARGET_PLATFORM STREQUAL "iOS" OR VNE_TARGET_PLATFORM STREQUAL "Android"
   OR VNE_TARGET_PLATFORM STREQUAL "visionOS" OR VNE_TARGET_PLATFORM STREQUAL "Web")
    target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_ES3)
endif()

target_link_libraries(imgui PUBLIC glfw glad::glad)
