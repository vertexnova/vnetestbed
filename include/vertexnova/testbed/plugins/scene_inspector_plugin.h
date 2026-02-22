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
 * @file scene_inspector_plugin.h
 * @brief Plugin that provides the scene graph inspector layer.
 *
 * Implements IPlugin; creates SceneInspectorLayer on demand.
 * Register with REGISTER_PLUGIN(SceneInspectorPlugin) in a .cpp.
 */

#include "vertexnova/testbed/plugin.h"

#include <memory>
#include <string>
#include <vector>

namespace vne {
namespace testbed {

/**
 * @class SceneInspectorPlugin
 * @brief Plugin that creates SceneInspectorLayer.
 */
class SceneInspectorPlugin : public IPlugin {
   public:
    std::string getName() const override;
    std::vector<std::unique_ptr<ILayer>> createLayers() override;
};

}  // namespace testbed
}  // namespace vne
