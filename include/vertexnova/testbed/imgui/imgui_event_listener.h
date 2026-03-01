#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * ImGui event listener — forwards vne::events to ImGui for manual input handling.
 * Mirrors vertexnova samples/core/imgui/imgui_event_listener pattern.
 *
 * Register with EventManager in ImGuiLayer::onAttach. Use with
 * ImGui_ImplGlfw_InitForOpenGL(window, false) so ImGui gets input from
 * this listener instead of GLFW callbacks.
 * ----------------------------------------------------------------------
 */

#if !defined(VNE_TESTBED_IMGUI)
#error "imgui_event_listener.h requires VNE_TESTBED_IMGUI."
#endif

#include "vertexnova/events/event_listener.h"

#include <memory>

namespace vne {
namespace testbed {

class ImGuiLayer;

/**
 * @class ImGuiEventListener
 * @brief Forwards vne::events to ImGui (io.AddKeyEvent, io.AddMouseButtonEvent, etc.).
 *
 * Used for unified event handling across platforms. ImGuiLayer creates and
 * registers this listener in onAttach, unregisters in onDetach.
 */
class ImGuiEventListener : public vne::events::EventListener {
   public:
    explicit ImGuiEventListener(ImGuiLayer* layer = nullptr);
    ~ImGuiEventListener() override;

    void onEvent(const vne::events::Event& event) override;

    void setImGuiLayer(ImGuiLayer* layer) { layer_ = layer; }

   private:
    ImGuiLayer* layer_;
};

}  // namespace testbed
}  // namespace vne
