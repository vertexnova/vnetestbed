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
 *
 * Optional: for static registration. Plugins create layers on demand;
 * use createAndPushLayers(stack) to populate a LayerStack.
 */

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/layer_stack.h"
#include "vertexnova/testbed/plugin.h"

#include <memory>
#include <vector>

namespace vne {
namespace testbed {

/**
 * @class PluginRegistry
 * @brief Singleton registry of IPlugin instances; creates layers for LayerStack.
 */
class PluginRegistry {
   public:
    static PluginRegistry& instance();

    void registerPlugin(std::unique_ptr<IPlugin> plugin);

    /** @brief Create layers from all plugins and push them to the stack. */
    void createAndPushLayers(LayerStack& stack, AppContext& ctx);

    /** @brief Number of registered plugins (for tests). */
    [[nodiscard]] std::size_t getPluginCount() const { return plugins_.size(); }

    PluginRegistry(const PluginRegistry&) = delete;
    PluginRegistry& operator=(const PluginRegistry&) = delete;

   private:
    PluginRegistry() = default;
    std::vector<std::unique_ptr<IPlugin>> plugins_;
};

/**
 * @def REGISTER_PLUGIN(PluginClass)
 * @brief Register a default-constructed PluginClass at static init time. Use in one .cpp per plugin.
 */
#define REGISTER_PLUGIN(PluginClass)                                                                \
    static bool VNETESTBED_REG_##PluginClass = []() {                                               \
        ::vne::testbed::PluginRegistry::instance().registerPlugin(std::make_unique<PluginClass>()); \
        return true;                                                                                \
    }()

}  // namespace testbed
}  // namespace vne
