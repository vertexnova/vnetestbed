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
#include "vertexnova/testbed/plugins/scene_inspector_layer.h"
#include "vertexnova/testbed/plugins/scene_inspector_plugin.h"

namespace vne {
namespace testbed {

std::string SceneInspectorPlugin::getName() const {
    return "SceneInspector";
}

std::vector<std::unique_ptr<ILayer>> SceneInspectorPlugin::createLayers() {
    std::vector<std::unique_ptr<ILayer>> layers;
    layers.push_back(std::make_unique<SceneInspectorLayer>());
    return layers;
}

REGISTER_PLUGIN(SceneInspectorPlugin);

}  // namespace testbed
}  // namespace vne
