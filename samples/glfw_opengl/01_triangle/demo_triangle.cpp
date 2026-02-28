/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 01_triangle: one demo that pushes TriangleDemoLayer.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/application.h"
#include "vertexnova/testbed/demo_factory.h"
#include "vertexnova/testbed/plugins/triangle_demo_layer.h"

#include <memory>

namespace {

void RegisterTriangleDemo(vne::testbed::Application& app) {
    app.getLayerStack().pushLayer(std::make_unique<vne::testbed::TriangleDemoLayer>(), app.getAppContext());
}

}  // namespace

VNETESTBED_REGISTER_DEMO("triangle", RegisterTriangleDemo)
