#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

namespace vne::testbed {
class Application;
}
#ifdef VNE_TESTBED_IMGUI
namespace vne::testbed {
class ImGuiLayer;
}
#endif

namespace vne::samples::hello_testbed {

// ---------------------------------------------------------------------------
// HelloSettingsLayer — adds the demo-specific section to the Settings panel
// ---------------------------------------------------------------------------
class HelloSettingsLayer : public vne::testbed::ILayer {
   public:
    HelloSettingsLayer();

    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;

#ifdef VNE_TESTBED_IMGUI
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
#endif

   private:
#ifdef VNE_TESTBED_IMGUI
    void renderPanel();

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

void registerHelloTestbedDemo(vne::testbed::Application& app);

}  // namespace vne::samples::hello_testbed
