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

#include "vertexnova/testbed/plugin_registry.h"

namespace vne {
namespace testbed {

PluginRegistry& PluginRegistry::instance() {
    static PluginRegistry reg;
    return reg;
}

void PluginRegistry::registerPlugin(std::unique_ptr<IPlugin> plugin) {
    if (plugin) {
        plugins_.push_back(std::move(plugin));
    }
}

void PluginRegistry::createAndPushLayers(LayerStack& stack, AppContext& ctx) {
    for (auto& plugin : plugins_) {
        if (!plugin) {
            continue;
        }
        auto layers = plugin->createLayers();
        for (auto& layer : layers) {
            if (layer) {
                stack.pushLayer(std::move(layer), ctx);
            }
        }
    }
}

}  // namespace testbed
}  // namespace vne
