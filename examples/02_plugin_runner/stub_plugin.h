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

#include "vertexnova/testbed/plugin.h"

namespace vne {
namespace testbed {

/** Stub plugin to validate PluginManager lifecycle (onInit -> update/render/imGui -> onShutdown). */
struct StubPlugin : IPlugin {
    void onInit() override {}
    void onUpdate(float /*dt*/) override {}
    void onRender() override {}
    void onImGui() override {}
    void onShutdown() override {}
};

}  // namespace testbed
}  // namespace vne
