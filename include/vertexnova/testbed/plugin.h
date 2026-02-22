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
 * @file plugin.h
 * @brief Lifecycle interface for testbed plugins.
 * PluginManager invokes: onInit once, then each frame onUpdate -> onRender -> onImGui; onShutdown on exit.
 */

namespace vne {
namespace testbed_ns {

/**
 * @class IPlugin
 * @brief Lifecycle interface every testbed plugin must implement.
 *
 * Plugins are owned by PluginManager, which calls each hook in order.
 * Implementations should keep each callback focused: heavy setup in
 * onInit(), per-frame logic in onUpdate()/onRender(), UI in onImGui(),
 * and cleanup in onShutdown().
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /** @brief Called once after all plugins are registered, before the main loop. */
    virtual void onInit() = 0;

    /**
     * @brief Called every frame during the update phase.
     * @param dt Delta time in seconds since the previous frame.
     */
    virtual void onUpdate(float dt) = 0;

    /** @brief Called every frame during the render phase. */
    virtual void onRender() = 0;

    /** @brief Called every frame during the ImGui render phase. */
    virtual void onImGui() = 0;

    /** @brief Called once when the application is shutting down, in reverse registration order. */
    virtual void onShutdown() = 0;
};

}  // namespace testbed_ns
}  // namespace vne
