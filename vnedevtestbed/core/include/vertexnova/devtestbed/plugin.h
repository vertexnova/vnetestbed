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
 * @brief Plugin lifecycle interface for the dev testbed.
 * The runner invokes: onInit once, then each frame onUpdate -> onRender -> onGui; onShutdown on exit.
 */

namespace vne {
namespace devtestbed {

struct AppContext;

/**
 * @class IPlugin
 * @brief Interface for dev-testbed plugins (scene, interaction, events, window validation).
 */
struct IPlugin {
    virtual ~IPlugin() = default;

    virtual void onInit(AppContext& ctx) = 0;
    virtual void onUpdate(AppContext& ctx, double deltaTime) = 0;
    virtual void onRender(AppContext& ctx) = 0;
    virtual void onGui(AppContext& ctx) = 0;
    virtual void onShutdown(AppContext& ctx) = 0;
};

}  // namespace devtestbed
}  // namespace vne
