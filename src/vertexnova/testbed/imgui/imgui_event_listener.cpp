/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * ImGui event listener — forwards vne::events to ImGui.
 * Mirrors vertexnova samples/core/imgui/imgui_event_listener.cpp.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/imgui/imgui_event_listener.h"
#include "vertexnova/testbed/imgui/imgui_key_mapping.h"
#include "vertexnova/testbed/imgui/imgui_layer.h"

#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/window_event.h"

#include <imgui.h>

namespace vne {
namespace testbed {

ImGuiEventListener::ImGuiEventListener(ImGuiLayer* layer)
    : layer_(layer) {}

ImGuiEventListener::~ImGuiEventListener() = default;

void ImGuiEventListener::onEvent(const vne::events::Event& event) {
    if (!layer_ || !layer_->isInitialized()) {
        return;
    }

    using ET = vne::events::EventType;
    ImGuiIO& io = ImGui::GetIO();

    switch (event.type()) {
        case ET::eKeyPressed: {
            const auto& e = static_cast<const vne::events::KeyPressedEvent&>(event);
            ImGuiKey key = mapKeyCodeToImGui(e.keyCode());
            if (key != ImGuiKey_None) {
                io.AddKeyEvent(key, true);
            }
            updateImGuiModifiers(e.modifiers());
            break;
        }
        case ET::eKeyReleased: {
            const auto& e = static_cast<const vne::events::KeyReleasedEvent&>(event);
            ImGuiKey key = mapKeyCodeToImGui(e.keyCode());
            if (key != ImGuiKey_None) {
                io.AddKeyEvent(key, false);
            }
            updateImGuiModifiers(e.modifiers());
            break;
        }
        case ET::eKeyTyped: {
            const auto& e = static_cast<const vne::events::KeyTypedEvent&>(event);
            unsigned int c = static_cast<unsigned int>(e.keyCode());
            if (c > 0 && c < 0x10000 && (c >= 0x0020 || c == 0x0009 || c == 0x000A || c == 0x000D)) {
                io.AddInputCharacter(c);
            }
            break;
        }
        case ET::eMouseButtonPressed: {
            const auto& e = static_cast<const vne::events::MouseButtonPressedEvent&>(event);
            int btn = mapMouseButtonToImGui(e.button());
            if (btn >= 0) {
                io.AddMouseButtonEvent(btn, true);
            }
            break;
        }
        case ET::eMouseButtonReleased: {
            const auto& e = static_cast<const vne::events::MouseButtonReleasedEvent&>(event);
            int btn = mapMouseButtonToImGui(e.button());
            if (btn >= 0) {
                io.AddMouseButtonEvent(btn, false);
            }
            break;
        }
        case ET::eMouseMoved: {
            const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
            float mx = static_cast<float>(e.x());
            float my = static_cast<float>(e.y());
            layer_->clientMouseToImGuiScreen(mx, my);
            io.AddMousePosEvent(mx, my);
            break;
        }
        case ET::eMouseScrolled: {
            const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
            io.AddMouseWheelEvent(static_cast<float>(e.xOffset()), static_cast<float>(e.yOffset()));
            break;
        }
        case ET::eWindowResize: {
            const auto& e = static_cast<const vne::events::WindowResizeEvent&>(event);
            io.DisplaySize = ImVec2(static_cast<float>(e.width()), static_cast<float>(e.height()));
            break;
        }
        default:
            break;
    }
}

}  // namespace testbed
}  // namespace vne
