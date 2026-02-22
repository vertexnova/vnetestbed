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

#include "vertexnova/testbed/plugin_manager.h"

#include <cassert>

namespace vne {
namespace testbed_ns {

void PluginManager::addPlugin(std::unique_ptr<IPlugin> plugin) {
    assert(plugin != nullptr && "PluginManager::addPlugin: plugin must not be null");
    plugins_.push_back(std::move(plugin));
}

void PluginManager::init() {
    for (auto& plugin : plugins_) {
        plugin->onInit();
    }
}

void PluginManager::update(float dt) {
    for (auto& plugin : plugins_) {
        plugin->onUpdate(dt);
    }
}

void PluginManager::render() {
    for (auto& plugin : plugins_) {
        plugin->onRender();
    }
}

void PluginManager::imGui() {
    for (auto& plugin : plugins_) {
        plugin->onImGui();
    }
}

void PluginManager::shutdown() {
    for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
        (*it)->onShutdown();
    }
    plugins_.clear();
}

std::size_t PluginManager::pluginCount() const noexcept {
    return plugins_.size();
}

}  // namespace testbed_ns
}  // namespace vne
