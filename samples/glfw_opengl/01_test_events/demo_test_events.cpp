/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 01_test_events
 * ---------------------
 * Everything from 00_hello_testbed (grid + axes + orbit camera) plus a live
 * Events panel in the Settings sidebar.
 *
 * What you can test here:
 *   • Every vneevents type arrives correctly (keyboard, mouse button,
 *     mouse move, scroll, window resize)
 *   • Event ordering — move vs pressed vs released
 *   • EventManager queue depth (events/sec counter)
 *   • Input polling via InputManager::isKeyPressed() — shows held state
 *     separately from discrete events
 *
 * ImGui Settings panel sections:
 *   [Events]      last 20 events with type, data, and frame counter
 *   [Input Poll]  live key-held state for WASD + Space + Escape,
 *                 current mouse position, current scroll
 *   [Stats]       total events received, events this second
 *
 * Libraries exercised: vne::testbed, vne::scene, vne::events,
 *                      vne::interaction (optional)
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include "vertexnova/events/event.h"
#include "vertexnova/events/event_listener.h"
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/input/input_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/events/window_event.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

#include <deque>
#include <string>

// GLFW key codes for input polling display (same values GLFW uses)
#ifndef GLFW_KEY_W
#define GLFW_KEY_W 87
#define GLFW_KEY_A 65
#define GLFW_KEY_S 83
#define GLFW_KEY_D 68
#define GLFW_KEY_SPACE 32
#define GLFW_KEY_ESCAPE 256
#define GLFW_KEY_LEFT_SHIFT 340
#endif

namespace {

// Non-owning shared_ptr helper
vne::events::EventManager::ListenerPtr asListenerPtr(vne::events::EventListener* raw) {
    return {raw, [](vne::events::EventListener*) {}};
}

// ---------------------------------------------------------------------------
// EventsLayer — captures every event and exposes stats for the UI
// ---------------------------------------------------------------------------
class EventsLayer : public vne::testbed::ILayer, public vne::events::EventListener {
   public:
    static constexpr std::size_t kMaxLog = 20u;

    EventsLayer()
        : vne::testbed::ILayer("EventsLayer") {}

    void onAttach(vne::testbed::AppContext& /*ctx*/) override {
        auto& mgr = vne::events::EventManager::instance();
        auto self = asListenerPtr(this);
        mgr.registerListener(vne::events::EventType::eKeyPressed, self);
        mgr.registerListener(vne::events::EventType::eKeyReleased, self);
        mgr.registerListener(vne::events::EventType::eMouseButtonPressed, self);
        mgr.registerListener(vne::events::EventType::eMouseButtonReleased, self);
        mgr.registerListener(vne::events::EventType::eMouseMoved, self);
        mgr.registerListener(vne::events::EventType::eMouseScrolled, self);
        mgr.registerListener(vne::events::EventType::eWindowResize, self);
    }

    void onDetach() override {
        auto& mgr = vne::events::EventManager::instance();
        mgr.unregisterListener(vne::events::EventType::eKeyPressed, this);
        mgr.unregisterListener(vne::events::EventType::eKeyReleased, this);
        mgr.unregisterListener(vne::events::EventType::eMouseButtonPressed, this);
        mgr.unregisterListener(vne::events::EventType::eMouseButtonReleased, this);
        mgr.unregisterListener(vne::events::EventType::eMouseMoved, this);
        mgr.unregisterListener(vne::events::EventType::eMouseScrolled, this);
        mgr.unregisterListener(vne::events::EventType::eWindowResize, this);
        log_.clear();
    }

    void onUpdate(float dt) override {
        // Rolling events-per-second counter
        events_this_second_ += events_since_last_update_;
        events_since_last_update_ = 0;
        second_acc_ += dt;
        if (second_acc_ >= 1.0f) {
            events_per_second_ = events_this_second_;
            events_this_second_ = 0;
            second_acc_ -= 1.0f;
        }
        frame_++;
    }

    void onEvent(const vne::events::Event& event) override {
        events_total_++;
        events_since_last_update_++;

        // Build a short display string: "type  data"
        std::string line;
        using ET = vne::events::EventType;
        switch (event.type()) {
            case ET::eKeyPressed: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                line = "KeyPressed    key=" + std::to_string(static_cast<int>(e.keyCode()));
                break;
            }
            case ET::eKeyRepeat: {
                const auto& e = static_cast<const vne::events::KeyRepeatEvent&>(event);
                line = "KeyRepeat     key=" + std::to_string(static_cast<int>(e.keyCode()))
                       + "  count=" + std::to_string(e.repeatCount());
                break;
            }
            case ET::eKeyReleased: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                line = "KeyReleased   key=" + std::to_string(static_cast<int>(e.keyCode()));
                break;
            }
            case ET::eMouseButtonPressed: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                line = "MousePressed  btn=" + std::to_string(static_cast<int>(e.button()));
                break;
            }
            case ET::eMouseButtonReleased: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                line = "MouseReleased btn=" + std::to_string(static_cast<int>(e.button()));
                break;
            }
            case ET::eMouseMoved: {
                const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                line = "MouseMoved    x=" + std::to_string(static_cast<int>(e.x()))
                       + "  y=" + std::to_string(static_cast<int>(e.y()));
                break;
            }
            case ET::eMouseScrolled: {
                const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
                line = "MouseScrolled dx=" + std::to_string(static_cast<int>(e.xOffset()))
                       + "  dy=" + std::to_string(static_cast<int>(e.yOffset()));
                break;
            }
            case ET::eWindowResize: {
                const auto& e = static_cast<const vne::events::WindowResizeEvent&>(event);
                line = "WindowResize  w=" + std::to_string(e.width()) + "  h=" + std::to_string(e.height());
                break;
            }
            default:
                line = event.toString();
                break;
        }

        // Prepend frame number
        line = "[f" + std::to_string(frame_) + "] " + line;
        log_.push_back(std::move(line));
        if (log_.size() > kMaxLog) {
            log_.pop_front();
        }
    }

    // Accessors for the UI layer
    [[nodiscard]] const std::deque<std::string>& log() const { return log_; }
    [[nodiscard]] uint64_t totalEvents() const { return events_total_; }
    [[nodiscard]] uint32_t eventsPerSecond() const { return events_per_second_; }

   private:
    std::deque<std::string> log_;
    uint64_t events_total_{0};
    uint32_t events_since_last_update_{0};
    uint32_t events_this_second_{0};
    uint32_t events_per_second_{0};
    float second_acc_{0.0f};
    uint64_t frame_{0};
};

// ---------------------------------------------------------------------------
// EventsSettingsLayer — ImGui panel; reads from EventsLayer + InputManager
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI
class EventsSettingsLayer : public vne::testbed::ILayer {
   public:
    EventsSettingsLayer()
        : vne::testbed::ILayer("EventsSettingsLayer") {
        setRenderSortKey(999);
    }

    void setImGuiLayer(vne::testbed::ImGuiLayer* l) { imgui_layer_ = l; }
    void setEventsLayer(EventsLayer* l) { events_layer_ = l; }

    void onAttach(vne::testbed::AppContext& /*ctx*/) override {
        if (imgui_layer_) {
            imgui_layer_->setSettingsCallback([this]() { renderPanel(); });
        }
    }

    void onDetach() override {
        if (imgui_layer_) {
            imgui_layer_->setSettingsCallback(nullptr);
            imgui_layer_ = nullptr;
        }
        events_layer_ = nullptr;
    }

   private:
    static void keyPollRow(const char* label, int key) {
        const bool held = vne::events::InputManager::isKeyPressed(key);
        const bool just_on = vne::events::InputManager::isKeyJustPressed(key);
        const bool just_off = vne::events::InputManager::isKeyJustReleased(key);
        const char* state = just_on ? "JUST ON" : (just_off ? "JUST OFF" : (held ? "held" : "—"));
        ImVec4 col = held ? ImVec4(0.2f, 1.0f, 0.2f, 1.f) : ImVec4(0.5f, 0.5f, 0.5f, 1.f);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", label);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(col, "%s", state);
    }

    void renderPanel() {
        // ---- Event log ----
        if (ImGui::CollapsingHeader("Events (last 20)", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (events_layer_) {
                ImGui::Text("Total: %llu  /sec: %u",
                            static_cast<unsigned long long>(events_layer_->totalEvents()),
                            events_layer_->eventsPerSecond());
                ImGui::Separator();

                const float log_height = ImGui::GetTextLineHeightWithSpacing() * 8.0f;
                ImGui::BeginChild("EventLog", ImVec2(0.f, log_height), true);
                // Show newest at top
                const auto& log = events_layer_->log();
                for (auto it = log.rbegin(); it != log.rend(); ++it) {
                    ImGui::TextUnformatted(it->c_str());
                }
                ImGui::EndChild();
            }
        }

        // ---- Input polling ----
        if (ImGui::CollapsingHeader("Input Polling", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("PollTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 80.f);
                ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                keyPollRow("W", GLFW_KEY_W);
                keyPollRow("A", GLFW_KEY_A);
                keyPollRow("S", GLFW_KEY_S);
                keyPollRow("D", GLFW_KEY_D);
                keyPollRow("Space", GLFW_KEY_SPACE);
                keyPollRow("Escape", GLFW_KEY_ESCAPE);
                keyPollRow("Shift", GLFW_KEY_LEFT_SHIFT);

                ImGui::EndTable();
            }
            ImGui::Spacing();

            // Mouse state from InputManager
            auto [mx, my] = vne::events::InputManager::mousePosition();
            auto [sx, sy] = vne::events::InputManager::mouseScroll();
            ImGui::Text("Mouse pos:    %d, %d", mx, my);
            ImGui::Text("Mouse scroll: %.2f, %.2f", static_cast<double>(sx), static_cast<double>(sy));
            ImGui::Text("LMB: %s   RMB: %s   MMB: %s",
                        vne::events::InputManager::isMouseButtonPressed(0) ? "down" : "up",
                        vne::events::InputManager::isMouseButtonPressed(1) ? "down" : "up",
                        vne::events::InputManager::isMouseButtonPressed(2) ? "down" : "up");
        }
    }

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    EventsLayer* events_layer_{nullptr};
};
#endif  // VNE_TESTBED_IMGUI

// ---------------------------------------------------------------------------

void RegisterTestEventsDemo(vne::testbed::Application& app) {
    // Layer 1: grid + axes + perspective camera
    auto* scene = new BaseSceneLayer("TestEventsBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

#ifdef VNE_TESTBED_INTERACTION
    // Layer 2: orbit-arcball interaction
    auto* interaction = new BaseInteractionLayer("TestEventsInteractionLayer");
    interaction->setCamera(scene->getCamera());
    app.getLayerStack().pushLayer(std::unique_ptr<BaseInteractionLayer>(interaction), app.getAppContext());
#endif

    // Layer 3: event capture
    auto* events_layer = new EventsLayer();
    app.getLayerStack().pushLayer(std::unique_ptr<EventsLayer>(events_layer), app.getAppContext());

#ifdef VNE_TESTBED_IMGUI
    // Layer 4: Settings panel
    auto* settings = new EventsSettingsLayer();
    auto* imgui = dynamic_cast<vne::testbed::ImGuiLayer*>(app.getLayerStack().findLayerByName("ImGuiLayer"));
    if (imgui) {
        settings->setImGuiLayer(imgui);
    }
    settings->setEventsLayer(events_layer);
    app.getLayerStack().pushLayer(std::unique_ptr<EventsSettingsLayer>(settings), app.getAppContext());
#endif
}

}  // namespace

VNETESTBED_REGISTER_DEMO("test_events", RegisterTestEventsDemo)
