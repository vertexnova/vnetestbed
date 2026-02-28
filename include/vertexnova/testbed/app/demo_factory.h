#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file app/demo_factory.h
 * @brief Registry of demo installers; one demo per sample executable via VNETESTBED_REGISTER_DEMO.
 *
 * Usage in a sample .cpp:
 *   void RegisterMyDemo(Application& app) {
 *     app.getLayerStack().pushLayer(std::make_unique<MyLayer>(), app.getAppContext());
 *   }
 *   VNETESTBED_REGISTER_DEMO("my_demo", RegisterMyDemo);
 *
 * DemoApplication::initialize() calls DemoFactory::CreateDemo(*this) (single registered demo)
 * or CreateDefault(*this) (e.g. first registered).
 */

#include "vertexnova/testbed/app/application.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vne {
namespace testbed {

/// Installer signature: pushes layers for this demo onto the app's LayerStack.
using DemoInstallFn = std::function<void(Application&)>;

/**
 * @class DemoFactory
 * @brief Static registry of demo installers keyed by string id.
 */
class DemoFactory {
   public:
    /// Register a demo by id and installer.
    static void registerDemo(const std::string& id, DemoInstallFn fn) { getMap()[id] = std::move(fn); }

    /// Install the demo with the given id. Returns false if id not found.
    static bool createDemo(Application& app, const std::string& id) {
        auto& map = getMap();
        auto it = map.find(id);
        if (it == map.end()) {
            return false;
        }
        it->second(app);
        return true;
    }

    /**
     * @brief Install the only registered demo. Succeeds only if exactly one demo is registered.
     */
    static bool createDemo(Application& app) {
        auto& map = getMap();
        if (map.size() != 1) {
            return false;
        }
        map.begin()->second(app);
        return true;
    }

    /**
     * @brief Default: install "window" if present, otherwise the first registered demo.
     */
    static bool createDefault(Application& app) {
        auto& map = getMap();
        if (map.empty()) {
            return false;
        }
        auto it = map.find("window");
        if (it != map.end()) {
            it->second(app);
            return true;
        }
        map.begin()->second(app);
        return true;
    }

    /// List all registered demo ids.
    static std::vector<std::string> list() {
        std::vector<std::string> out;
        out.reserve(getMap().size());
        for (const auto& kv : getMap()) {
            out.push_back(kv.first);
        }
        return out;
    }

   private:
    static std::unordered_map<std::string, DemoInstallFn>& getMap() {
        static std::unordered_map<std::string, DemoInstallFn> map;
        return map;
    }
};

#ifndef VNE_CAT
#define VNE_CAT_IMPL(a, b) a##b
#define VNE_CAT(a, b) VNE_CAT_IMPL(a, b)
#endif

/**
 * @def VNETESTBED_REGISTER_DEMO(id_str, install_fn)
 * @brief Register a demo at static init. Use at the end of the sample .cpp that defines the installer.
 */
#define VNETESTBED_REGISTER_DEMO(ID_STR, INSTALL_FN) \
    static ::vne::testbed::DemoAutoRegister VNE_CAT(_vnetestbed_demo_, __LINE__){ID_STR, INSTALL_FN};

/** Helper for static registration. */
struct DemoAutoRegister {
    DemoAutoRegister(const char* id, DemoInstallFn fn) { DemoFactory::registerDemo(id, std::move(fn)); }
};

}  // namespace testbed
}  // namespace vne
