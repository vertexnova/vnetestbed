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
 * @brief Plugin interface for extensibility and discovery.
 *
 * Plugins are registered at static init and produce layers on demand.
 * Plugin = extensibility; Layer = runtime execution.
 */

#include "vertexnova/testbed/layer.h"

#include <memory>
#include <string>
#include <vector>

namespace vne {
namespace testbed {

/**
 * @class IPlugin
 * @brief Interface for discoverable plugins that create layers.
 *
 * Plugins register via REGISTER_PLUGIN; the runner calls createAndPushLayers
 * to populate the LayerStack from all registered plugins.
 */
class IPlugin {
   public:
    virtual ~IPlugin() = default;

    IPlugin(const IPlugin&) = delete;
    IPlugin& operator=(const IPlugin&) = delete;
    IPlugin(IPlugin&&) = delete;
    IPlugin& operator=(IPlugin&&) = delete;

    /** @brief Plugin display name. */
    virtual std::string getName() const = 0;

    /** @brief Create layers this plugin contributes. May return one or more. */
    virtual std::vector<std::unique_ptr<ILayer>> createLayers() = 0;

   protected:
    IPlugin() = default;
};

}  // namespace testbed
}  // namespace vne
