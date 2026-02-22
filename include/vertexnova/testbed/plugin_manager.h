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
 * @file plugin_manager.h
 * @brief Owns and drives the lifecycle of all registered plugins.
 *
 * Plugins are stored in registration order.  All forward-phase hooks
 * (onInit, onUpdate, onRender, onImGui) are dispatched in that order;
 * onShutdown is dispatched in reverse order so that a plugin that
 * depends on another is torn down first.
 *
 * Example:
 * @code
 *   PluginManager mgr;
 *   mgr.addPlugin(std::make_unique<SceneInspectorPlugin>());
 *   mgr.init();
 *   // main loop
 *   mgr.update(dt);
 *   mgr.render();
 *   mgr.imGui();
 *   // on exit
 *   mgr.shutdown();
 * @endcode
 */

#include "vertexnova/testbed/plugin.h"

#include <memory>
#include <vector>

namespace vne {
namespace testbed {

/**
 * @class PluginManager
 * @brief Owns and drives the lifecycle of all registered plugins.
 */
class PluginManager {
   public:
    PluginManager() = default;
    ~PluginManager() = default;

    // Non-copyable, movable
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = default;
    PluginManager& operator=(PluginManager&&) = default;

    /**
     * @brief Register a plugin and transfer ownership to this manager.
     * @param plugin  Plugin instance; must not be null.
     */
    void addPlugin(std::unique_ptr<IPlugin> plugin);

    /** @brief Call onInit() on all plugins in registration order. */
    void init();

    /**
     * @brief Call onUpdate(dt) on all plugins in registration order.
     * @param dt Delta time in seconds.
     */
    void update(float dt);

    /** @brief Call onRender() on all plugins in registration order. */
    void render();

    /** @brief Call onImGui() on all plugins in registration order. */
    void imGui();

    /**
     * @brief Call onShutdown() on all plugins in reverse registration order.
     *
     * After this call the plugin list is cleared.
     */
    void shutdown();

    /** @brief Number of currently registered plugins. */
    [[nodiscard]] std::size_t pluginCount() const noexcept;

   private:
    std::vector<std::unique_ptr<IPlugin>> plugins_;
};

}  // namespace testbed
}  // namespace vne
