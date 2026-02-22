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

void PluginRegistry::registerPlugin(std::string name, std::unique_ptr<ILayer> layer) {
    plugins_.emplace_back(std::move(name), std::move(layer));
}

std::vector<ILayer*> PluginRegistry::getPlugins() {
    std::vector<ILayer*> out;
    out.reserve(plugins_.size());
    for (auto& p : plugins_) {
        out.push_back(p.second.get());
    }
    return out;
}

}  // namespace testbed
}  // namespace vne
