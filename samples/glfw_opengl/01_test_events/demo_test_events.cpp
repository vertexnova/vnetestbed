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

#include "demo_test_events.h"

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/window/glfw_window.h"

#include "vertexnova/events/input/input_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/touch_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/events/window_event.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

#include <cstring>
#include <ranges>
#include <string>

namespace {

constexpr int kRenderSortKey = 999;
constexpr float kFpsIntervalSec = 1.0f;

constexpr float kKeyHeldColorR = 0.9f;
constexpr float kKeyHeldColorG = 0.3f;
constexpr float kKeyHeldColorB = 0.15f;
constexpr float kKeyIdleColor = 0.5f;

constexpr float kEventLogVisibleLines = 8.0f;
constexpr float kPollTableKeyColumnWidthPx = 80.0f;

constexpr int kMouseButtonLeft = 0;
constexpr int kMouseButtonRight = 1;
constexpr int kMouseButtonMiddle = 2;

constexpr std::size_t kMaxLog = 20;  //!< Maximum number of log lines
constexpr int kInvalidKeyCode = -1;  //!< Invalid key code

#ifdef VNE_TESTBED_IMGUI
constexpr size_t kCharDisplayLastN = 10;
#endif

}  // namespace

namespace vne::samples {

// ---------------------------------------------------------------------------
// EventsLayer
// ---------------------------------------------------------------------------

EventsLayer::EventsLayer()
    : vne::testbed::ILayer("EventsLayer") {}

void EventsLayer::onAttach(vne::testbed::AppContext& /*app_context*/) {}

void EventsLayer::onDetach() {
    log_.clear();
}

void EventsLayer::onUpdate(float dt) {
    // Events arrive in onEvent() into events_since_last_update_; each frame we fold that into
    // events_this_second_. When second_acc_ reaches 1s, copy the count to events_per_second_
    // (for the UI) and start a new second.
    events_this_second_ += events_since_last_update_;
    events_since_last_update_ = 0;
    second_acc_ += dt;
    if (second_acc_ >= kFpsIntervalSec) {
        events_per_second_ = events_this_second_;
        events_this_second_ = 0;
        second_acc_ -= kFpsIntervalSec;
    }
    frame_++;
}

void EventsLayer::onEvent(const vne::events::Event& event) {
    events_total_++;
    events_since_last_update_++;

    std::string line;
    switch (event.type()) {
        case vne::events::EventType::eKeyPressed: {
            const auto& e = static_cast<const vne::events::KeyEvent&>(event);
            setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::ePressed);
            line = "KeyPressed    key=" + std::to_string(static_cast<int>(e.keyCode()));
            break;
        }
        case vne::events::EventType::eKeyRepeat: {
            const auto& e = static_cast<const vne::events::KeyRepeatEvent&>(event);
            setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::eRepeat);
            line = "KeyRepeat     key=" + std::to_string(static_cast<int>(e.keyCode()))
                   + "  count=" + std::to_string(e.repeatCount());
            break;
        }
        case vne::events::EventType::eKeyReleased: {
            const auto& e = static_cast<const vne::events::KeyEvent&>(event);
            setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::eReleased);
            line = "KeyReleased   key=" + std::to_string(static_cast<int>(e.keyCode()));
            break;
        }
        case vne::events::EventType::eMouseButtonPressed: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            line = "MousePressed  btn=" + std::to_string(static_cast<int>(e.button()));
            break;
        }
        case vne::events::EventType::eMouseButtonReleased: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            line = "MouseReleased btn=" + std::to_string(static_cast<int>(e.button()));
            break;
        }
        case vne::events::EventType::eMouseMoved: {
            const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
            line = "MouseMoved    x=" + std::to_string(static_cast<int>(e.x()))
                   + "  y=" + std::to_string(static_cast<int>(e.y()));
            break;
        }
        case vne::events::EventType::eMouseScrolled: {
            const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
            line = "MouseScrolled dx=" + std::to_string(static_cast<int>(e.xOffset()))
                   + "  dy=" + std::to_string(static_cast<int>(e.yOffset()));
            break;
        }
        case vne::events::EventType::eWindowResize: {
            const auto& e = static_cast<const vne::events::WindowResizeEvent&>(event);
            line = "WindowResize  w=" + std::to_string(e.width()) + "  h=" + std::to_string(e.height());
            break;
        }
        case vne::events::EventType::eWindowClose:
            line = "WindowClose";
            break;
        case vne::events::EventType::eTouchPress: {
            const auto& e = static_cast<const vne::events::TouchPressEvent&>(event);
            setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::ePress);
            line = "TouchPress   id=" + std::to_string(e.touchId()) + "  x=" + std::to_string(static_cast<int>(e.x()))
                   + "  y=" + std::to_string(static_cast<int>(e.y()));
            break;
        }
        case vne::events::EventType::eTouchRelease: {
            const auto& e = static_cast<const vne::events::TouchReleaseEvent&>(event);
            setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::eRelease);
            line = "TouchRelease id=" + std::to_string(e.touchId()) + "  x=" + std::to_string(static_cast<int>(e.x()))
                   + "  y=" + std::to_string(static_cast<int>(e.y()));
            break;
        }
        case vne::events::EventType::eTouchMove: {
            const auto& e = static_cast<const vne::events::TouchMoveEvent&>(event);
            setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::eMove);
            line = "TouchMove    id=" + std::to_string(e.touchId()) + "  x=" + std::to_string(static_cast<int>(e.x()))
                   + "  y=" + std::to_string(static_cast<int>(e.y()));
            break;
        }
        default:
            line = event.toString();
            break;
    }

    line = "[f" + std::to_string(frame_) + "] " + line;
    log_.push_back(std::move(line));
    if (log_.size() > kMaxLog) {
        log_.pop_front();
    }
}

void EventsLayer::setLastKey(int key_code, LastKeyAction action) {
    last_key_code_ = key_code;
    last_key_action_ = action;
}

void EventsLayer::setLastTouch(uint32_t id, double x, double y, LastTouchAction action) {
    last_touch_id_ = id;
    last_touch_x_ = x;
    last_touch_y_ = y;
    last_touch_action_ = action;
}

// ---------------------------------------------------------------------------
// EventsSettingsLayer
// ---------------------------------------------------------------------------

#ifdef VNE_TESTBED_IMGUI
EventsSettingsLayer::EventsSettingsLayer()
    : vne::testbed::ILayer("EventsSettingsLayer") {
    setRenderSortKey(kRenderSortKey);
}

void EventsSettingsLayer::setImGuiLayer(vne::testbed::ImGuiLayer* layer) {
    imgui_layer_ = layer;
}

void EventsSettingsLayer::setEventsLayer(EventsLayer* layer) {
    events_layer_ = layer;
}

void EventsSettingsLayer::onAttach(vne::testbed::AppContext& /*app_context*/) {
    if (imgui_layer_) {
        imgui_layer_->setSettingsCallback([this]() { renderPanel(); });
    }
}

void EventsSettingsLayer::onDetach() {
    if (imgui_layer_) {
        imgui_layer_->setSettingsCallback(nullptr);
        imgui_layer_ = nullptr;
    }
    events_layer_ = nullptr;
}

const char* EventsSettingsLayer::keyCodeToLabel(int key_code) {
    switch (static_cast<vne::events::KeyCode>(key_code)) {
        case vne::events::KeyCode::eW:
            return "W";
        case vne::events::KeyCode::eA:
            return "A";
        case vne::events::KeyCode::eS:
            return "S";
        case vne::events::KeyCode::eD:
            return "D";
        case vne::events::KeyCode::eSpace:
            return "Space";
        case vne::events::KeyCode::eEscape:
            return "Escape";
        case vne::events::KeyCode::eLeftShift:
            return "Shift";
        default:
            return nullptr;
    }
}

void EventsSettingsLayer::keyPollRow(const char* label, int key) {
    const bool held = vne::events::InputManager::isKeyPressed(key);
    const bool just_on = vne::events::InputManager::isKeyJustPressed(key);
    const bool just_off = vne::events::InputManager::isKeyJustReleased(key);
    const char* state = just_on ? "JUST ON" : (just_off ? "JUST OFF" : (held ? "held" : "—"));
    ImVec4 col = held ? ImVec4(kKeyHeldColorR, kKeyHeldColorG, kKeyHeldColorB, 1.0f)
                      : ImVec4(kKeyIdleColor, kKeyIdleColor, kKeyIdleColor, 1.0f);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextColored(col, "%s", state);
}

void EventsSettingsLayer::renderPanel() {
    // ---- Event log: plain list, newest first ----
    const std::string events_header = "Events (last " + std::to_string(kMaxLog) + ")";
    if (ImGui::CollapsingHeader(events_header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (events_layer_) {
            ImGui::Text("Total: %llu  /sec: %u",
                        static_cast<unsigned long long>(events_layer_->totalEvents()),
                        events_layer_->eventsPerSecond());
            ImGui::Separator();

            const float log_height = ImGui::GetTextLineHeightWithSpacing() * kEventLogVisibleLines;
            ImGui::BeginChild("EventLog", ImVec2(0.0f, log_height), true);
            const auto& log = events_layer_->log();
            for (const auto& line : std::ranges::reverse_view(log)) {
                ImGui::TextUnformatted(line.c_str());
            }
            ImGui::EndChild();
        }
    }

    // ---- Window size & active viewport ----
    if (ImGui::CollapsingHeader("Window & viewports", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto [win_w, win_h] = vne::events::InputManager::windowSize();
        ImGui::Text("Window size: %d x %d", win_w, win_h);
        if (imgui_layer_) {
            const int vp_count = imgui_layer_->getViewportCount();
            ImGui::Text("Viewports: %d", vp_count);
            auto [mx, my] = vne::events::InputManager::mousePosition();
            const int hovered = imgui_layer_->getHoveredViewportIndex(static_cast<float>(mx), static_cast<float>(my));
            const std::string active_name = (hovered >= 0) ? imgui_layer_->getViewportName(hovered) : std::string("—");
            ImGui::Text("Active viewport: %s", active_name.c_str());
        }
    }

    // ---- Input polling ----
    if (ImGui::CollapsingHeader("Input Polling", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Keyboard: key table + key press/release display + char entry
        ImGui::Text("Keyboard");
        ImGui::Separator();
        if (ImGui::BeginTable("PollTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, kPollTableKeyColumnWidthPx);
            ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            keyPollRow("W", static_cast<int>(vne::events::KeyCode::eW));
            keyPollRow("A", static_cast<int>(vne::events::KeyCode::eA));
            keyPollRow("S", static_cast<int>(vne::events::KeyCode::eS));
            keyPollRow("D", static_cast<int>(vne::events::KeyCode::eD));
            keyPollRow("Space", static_cast<int>(vne::events::KeyCode::eSpace));
            keyPollRow("Escape", static_cast<int>(vne::events::KeyCode::eEscape));
            keyPollRow("Shift", static_cast<int>(vne::events::KeyCode::eLeftShift));

            ImGui::EndTable();
        }
        if (events_layer_) {
            const int kc = events_layer_->lastKeyCode();
            const auto ka = events_layer_->lastKeyAction();
            const char* action_str = (ka == LastKeyAction::ePressed)    ? "pressed"
                                     : (ka == LastKeyAction::eReleased) ? "released"
                                     : (ka == LastKeyAction::eRepeat)   ? "repeat"
                                                                        : "";
            const char* key_label = keyCodeToLabel(kc);
            if (action_str[0] != '\0' && kc != kInvalidKeyCode) {
                if (key_label) {
                    ImGui::Text("Key: %s %s", key_label, action_str);
                } else {
                    ImGui::Text("Key: %d %s", kc, action_str);
                }
            }
        }
        ImGui::Spacing();
        ImGui::Text("Char entry");
        ImGui::Separator();
        ImGui::InputText("Char display", char_display_buf_, sizeof(char_display_buf_));
        const size_t len = std::strlen(char_display_buf_);
        const char* last_n =
            (len <= kCharDisplayLastN) ? char_display_buf_ : (char_display_buf_ + len - kCharDisplayLastN);
        ImGui::Text("Last %zu: %s", kCharDisplayLastN, len > 0u ? last_n : "(none)");

        // Mouse: collapsing header
        if (ImGui::CollapsingHeader("Mouse", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto [mx, my] = vne::events::InputManager::mousePosition();
            auto [sx, sy] = vne::events::InputManager::mouseScroll();
            ImGui::Text("Mouse pos:    %d, %d", mx, my);
            ImGui::Text("Mouse scroll: X %.2f  Y %.2f", static_cast<double>(sx), static_cast<double>(sy));
            const char* lb = vne::events::InputManager::isMouseButtonPressed(kMouseButtonLeft) ? "down" : "up  ";
            const char* rb = vne::events::InputManager::isMouseButtonPressed(kMouseButtonRight) ? "down" : "up  ";
            const char* mb = vne::events::InputManager::isMouseButtonPressed(kMouseButtonMiddle) ? "down" : "up  ";
            ImGui::Text("LMB: %s   RMB: %s   MMB: %s", lb, rb, mb);
        }

        // Touch: LMB emulates touch id 0 on desktop; show last touch on panel
        if (ImGui::CollapsingHeader("Touch", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextUnformatted("LMB = touch id 0");
            if (events_layer_) {
                const uint32_t tid = events_layer_->lastTouchId();
                const double tx = events_layer_->lastTouchX();
                const double ty = events_layer_->lastTouchY();
                const auto ta = events_layer_->lastTouchAction();
                const char* action_str = (ta == LastTouchAction::ePress)     ? "press"
                                         : (ta == LastTouchAction::eRelease) ? "release"
                                         : (ta == LastTouchAction::eMove)    ? "move"
                                                                             : "";
                if (action_str[0] != '\0') {
                    ImGui::Text("Last: id %u  %s  (%.0f, %.0f)", tid, action_str, tx, ty);
                }
            }
        }
    }
}
#endif  // VNE_TESTBED_IMGUI

// ---------------------------------------------------------------------------

void registerTestEventsDemo(vne::testbed::Application& app) {
    // Enable LMB → touch event synthesis for this demo (Events panel shows touch press/move/release).
    if (auto* glfw_win = dynamic_cast<vne::testbed::window::GlfwWindow*>(app.getAppContext().window)) {
        glfw_win->setTouchEmulationEnabled(true);
    }
    // Layer 1: grid + axes + perspective camera
    auto* scene = new BaseSceneLayer("TestEventsBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

#ifdef VNE_TESTBED_INTERACTION
    // Layer 2: orbit/trackball interaction (per-viewport cameras when using multiple viewports)
    auto* interaction = new BaseInteractionLayer("TestEventsInteractionLayer");
    interaction->setSceneLayer(scene);
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
#ifdef VNE_TESTBED_INTERACTION
        interaction->setImGuiLayer(imgui);
#endif
    }
    settings->setEventsLayer(events_layer);
    app.getLayerStack().pushLayer(std::unique_ptr<EventsSettingsLayer>(settings), app.getAppContext());
#endif
}

VNETESTBED_REGISTER_DEMO("test_events", registerTestEventsDemo)

}  // namespace vne::samples
