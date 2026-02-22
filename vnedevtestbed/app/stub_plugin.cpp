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

#include "vertexnova/devtestbed/app_context.h"
#include "vertexnova/devtestbed/plugin.h"
#include "vertexnova/devtestbed/plugin_registry.h"

namespace vne {
namespace devtestbed {

/** Stub plugin to validate registry and lifecycle (onInit -> update/render/gui -> onShutdown). */
struct StubPlugin : IPlugin {
    void onInit(AppContext&) override {}
    void onUpdate(AppContext&, double) override {}
    void onRender(AppContext&) override {}
    void onGui(AppContext&) override {}
    void onShutdown(AppContext&) override {}
};

REGISTER_PLUGIN(StubPlugin);

}  // namespace devtestbed
}  // namespace vne
