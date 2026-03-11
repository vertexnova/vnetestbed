#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Shared path helpers for glfw_opengl samples (testdata root, shader lookup).
 * Include from sample .cpp; ensure VNETESTBED_TESTDATA_DIR is defined in CMake
 * when the sample needs getTestdataPath().
 * ----------------------------------------------------------------------
 */

#include <filesystem>
#include <string>

namespace vne {
namespace samples {
namespace common {

/**
 * Resolve path under the vnetestbed testdata submodule (e.g. "resources/meshes/box.ply").
 * Uses VNETESTBED_TESTDATA_DIR when defined at compile time; otherwise returns empty string.
 */
inline std::string getTestdataPath(const std::string& subpath) {
#ifdef VNETESTBED_TESTDATA_DIR
    std::string root(VNETESTBED_TESTDATA_DIR);
    std::string p = subpath;
    while (!p.empty() && (p.front() == '/' || p.front() == '\\'))
        p.erase(0, 1);
    if (p.empty())
        return root;
    if (root.back() == '/' || root.back() == '\\')
        return root + p;
    return root + "/" + p;
#else
    (void)subpath;
    return {};
#endif
}

/**
 * Resolve shader path: try cwd and cwd/bin/samples (for running from build or bin/samples).
 */
inline std::filesystem::path resolveShaderPath(const char* filename) {
    std::error_code ec;
    std::filesystem::path p = std::filesystem::current_path(ec) / filename;
    if (!ec && std::filesystem::exists(p, ec))
        return p;
    p = std::filesystem::current_path(ec) / "bin/samples" / filename;
    if (!ec && std::filesystem::exists(p, ec))
        return p;
    return {};
}

}  // namespace common
}  // namespace samples
}  // namespace vne
