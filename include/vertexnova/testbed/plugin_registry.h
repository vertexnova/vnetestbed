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
 * @file plugin_registry.h
 * @brief Singleton registry for testbed plugins and REGISTER_PLUGIN macro.
 */

#include "vertexnova/testbed/plugin.h"

#include <memory>
#include <string>
#include <vector>

namespace vne {
namespace testbed {

/**
 * @class PluginRegistry
 * @brief Singleton registry of IPlugin instances; runner iterates to call lifecycle.
 */
class PluginRegistry {
   public:
    static PluginRegistry& instance();

    void registerPlugin(std::string name, std::unique_ptr<IPlugin> plugin);

    /** @brief Get all registered plugins in registration order. */
    std::vector<IPlugin*> getPlugins();

    PluginRegistry(const PluginRegistry&) = delete;
    PluginRegistry& operator=(const PluginRegistry&) = delete;

   private:
    PluginRegistry() = default;
    std::vector<std::pair<std::string, std::unique_ptr<IPlugin>>> plugins_;
};

/**
 * @def REGISTER_PLUGIN(PluginClass)
 * @brief Register a default-constructed PluginClass at static init time. Use in one .cpp per plugin.
 */
#define REGISTER_PLUGIN(PluginClass)                                                                              \
    static bool VNETESTBED_REG_##PluginClass = []() {                                                             \
        ::vne::testbed::PluginRegistry::instance().registerPlugin(#PluginClass, std::make_unique<PluginClass>()); \
        return true;                                                                                              \
    }()

}  // namespace testbed
}  // namespace vne
