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
 *     mouse move, scroll, window resize, window close, touch press/move/release)
 *   • Touch: LMB emulates touch id 0 (TouchPress, TouchMove, TouchRelease)
 *   • Event ordering — move vs pressed vs released
 *   • EventManager queue depth (events/sec counter)
 *   • Input polling via InputManager::isKeyPressed() — shows held state
 *     separately from discrete events
 *
 * ImGui Settings panel sections:
 *   [Events]      last N events (key, mouse, window resize/close, touch) with type, data, frame
 *   [Input Poll]  live key-held state for WASD + Space + Escape,
 *                 current mouse position, current scroll; Keyboard, Mouse, Char entry, Touch
 *   [Stats]       total events received, events this second
 *
 * Libraries exercised: vne::testbed, vne::scene, vne::events,
 *                      vne::interaction (optional)
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/layer.h"

#include "vertexnova/events/event.h"
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
#include <deque>
#include <ranges>
#include <string>

namespace {

constexpr int kRenderSortKey = 999;  //!< layer sorting order number

// Demo UI / timing constants
constexpr float kFpsIntervalSec = 1.0f;

// Orange
constexpr float kKeyHeldColorR = 0.9f;
constexpr float kKeyHeldColorG = 0.3f;
constexpr float kKeyHeldColorB = 0.15f;
// Gray
constexpr float kKeyIdleColor = 0.5f;

constexpr float kEventLogVisibleLines = 8.0f;
constexpr float kPollTableKeyColumnWidthPx = 80.0f;

constexpr int kMouseButtonLeft = 0;
constexpr int kMouseButtonRight = 1;
constexpr int kMouseButtonMiddle = 2;
constexpr int kInvalidKeyCode = -1;
constexpr std::size_t kEventLogMaxEntries = 20u;

enum class LastKeyAction { eNone = 0, ePressed = 1, eReleased = 2, eRepeat = 3 };

enum class LastTouchAction { eNone = 0, ePress = 1, eMove = 2, eRelease = 3 };

// ---------------------------------------------------------------------------
// EventsLayer — captures every event and exposes stats for the UI
// ---------------------------------------------------------------------------
class EventsLayer : public vne::testbed::ILayer {
   public:
    static constexpr std::size_t kMaxLog = kEventLogMaxEntries;

    EventsLayer()
        : vne::testbed::ILayer("EventsLayer") {}

    void onAttach(vne::testbed::AppContext& /*ctx*/) override {}

    void onDetach() override { log_.clear(); }

    void onUpdate(float dt) override {
        // Rolling events-per-second counter
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

    void onEvent(const vne::events::Event& event) override {
        events_total_++;
        events_since_last_update_++;

        std::string line;
        using ET = vne::events::EventType;
        switch (event.type()) {
            case ET::eKeyPressed: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::ePressed);
                line = "KeyPressed    key=" + std::to_string(static_cast<int>(e.keyCode()));
                break;
            }
            case ET::eKeyRepeat: {
                const auto& e = static_cast<const vne::events::KeyRepeatEvent&>(event);
                setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::eRepeat);
                line = "KeyRepeat     key=" + std::to_string(static_cast<int>(e.keyCode()))
                       + "  count=" + std::to_string(e.repeatCount());
                break;
            }
            case ET::eKeyReleased: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::eReleased);
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
            case ET::eWindowClose:
                line = "WindowClose";
                break;
            case ET::eTouchPress: {
                const auto& e = static_cast<const vne::events::TouchPressEvent&>(event);
                setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::ePress);
                line = "TouchPress   id=" + std::to_string(e.touchId()) + "  x="
                       + std::to_string(static_cast<int>(e.x())) + "  y=" + std::to_string(static_cast<int>(e.y()));
                break;
            }
            case ET::eTouchRelease: {
                const auto& e = static_cast<const vne::events::TouchReleaseEvent&>(event);
                setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::eRelease);
                line = "TouchRelease id=" + std::to_string(e.touchId()) + "  x="
                       + std::to_string(static_cast<int>(e.x())) + "  y=" + std::to_string(static_cast<int>(e.y()));
                break;
            }
            case ET::eTouchMove: {
                const auto& e = static_cast<const vne::events::TouchMoveEvent&>(event);
                setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::eMove);
                line = "TouchMove    id=" + std::to_string(e.touchId()) + "  x="
                       + std::to_string(static_cast<int>(e.x())) + "  y=" + std::to_string(static_cast<int>(e.y()));
                break;
            }
            default:
                line = event.toString();
                break;
        }

        line = "[f-" + std::to_string(frame_) + "] " + line;
        log_.push_back(std::move(line));
        if (log_.size() > kMaxLog) {
            log_.pop_front();
        }
    }

    // Accessors for the UI layer
    [[nodiscard]] const std::deque<std::string>& log() const { return log_; }
    [[nodiscard]] uint64_t totalEvents() const { return events_total_; }
    [[nodiscard]] uint32_t eventsPerSecond() const { return events_per_second_; }

    [[nodiscard]] int lastKeyCode() const { return last_key_code_; }
    [[nodiscard]] LastKeyAction lastKeyAction() const { return last_key_action_; }

    [[nodiscard]] uint32_t lastTouchId() const { return last_touch_id_; }
    [[nodiscard]] double lastTouchX() const { return last_touch_x_; }
    [[nodiscard]] double lastTouchY() const { return last_touch_y_; }
    [[nodiscard]] LastTouchAction lastTouchAction() const { return last_touch_action_; }

   private:
    void setLastKey(int key_code, LastKeyAction action) {
        last_key_code_ = key_code;
        last_key_action_ = action;
    }
    void setLastTouch(uint32_t id, double x, double y, LastTouchAction action) {
        last_touch_id_ = id;
        last_touch_x_ = x;
        last_touch_y_ = y;
        last_touch_action_ = action;
    }

    std::deque<std::string> log_;
    int last_key_code_{kInvalidKeyCode};
    LastKeyAction last_key_action_{LastKeyAction::eNone};
    uint32_t last_touch_id_{0};
    double last_touch_x_{0.0};
    double last_touch_y_{0.0};
    LastTouchAction last_touch_action_{LastTouchAction::eNone};
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
        setRenderSortKey(kRenderSortKey);
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
    static const char* keyCodeToLabel(int key_code) {
        using K = vne::events::KeyCode;
        switch (static_cast<K>(key_code)) {
            case K::eW:
                return "W";
            case K::eA:
                return "A";
            case K::eS:
                return "S";
            case K::eD:
                return "D";
            case K::eSpace:
                return "Space";
            case K::eEscape:
                return "Escape";
            case K::eLeftShift:
                return "Shift";
            default:
                return nullptr;
        }
    }

    static void keyPollRow(const char* label, int key) {
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

    void renderPanel() {
        // ---- Event log: plain list, newest first ----
        const std::string events_header = "Events (last " + std::to_string(kEventLogMaxEntries) + ")";
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
                const int hovered =
                    imgui_layer_->getHoveredViewportIndex(static_cast<float>(mx), static_cast<float>(my));
                const std::string active_name =
                    (hovered >= 0) ? imgui_layer_->getViewportName(hovered) : std::string("—");
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

    static constexpr size_t kCharDisplayBufSize = 256;
    static constexpr size_t kCharDisplayLastN = 10;
    char char_display_buf_[kCharDisplayBufSize] = {};

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    EventsLayer* events_layer_{nullptr};
};
#endif  // VNE_TESTBED_IMGUI

// ---------------------------------------------------------------------------

void registerTestEventsDemo(vne::testbed::Application& app) {
    // Layer 1: grid + axes + perspective camera
    auto* scene = new BaseSceneLayer("TestEventsBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

#ifdef VNE_TESTBED_INTERACTION
    // Layer 2: orbit-arcball interaction (per-viewport cameras when using 2 or 4 viewports)
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

}  // namespace

VNETESTBED_REGISTER_DEMO("test_events", registerTestEventsDemo)
