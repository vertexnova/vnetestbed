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

#include "vertexnova/devtestbed/plugin_registry.h"

namespace vne {
namespace devtestbed {

PluginRegistry& PluginRegistry::instance() {
    static PluginRegistry reg;
    return reg;
}

void PluginRegistry::registerPlugin(std::string name, std::unique_ptr<IPlugin> plugin) {
    plugins_.emplace_back(std::move(name), std::move(plugin));
}

std::vector<IPlugin*> PluginRegistry::getPlugins() {
    std::vector<IPlugin*> out;
    out.reserve(plugins_.size());
    for (auto& p : plugins_) {
        out.push_back(p.second.get());
    }
    return out;
}

}  // namespace devtestbed
}  // namespace vne
