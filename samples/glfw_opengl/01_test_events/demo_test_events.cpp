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
 *     mouse move, scroll, window resize, window close)
 *   • Event ordering — move vs pressed vs released
 *   • EventManager queue depth (events/sec counter)
 *   • Input polling via InputManager::isKeyPressed() — shows held state
 *     separately from discrete events
 *
 * ImGui Settings panel sections:
 *   [Events]      last 20 events (key, mouse, window resize/close) with type, data, frame
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
#include <string>

namespace {

constexpr int kRenderSortKey = 999;  //!< layer sorting order number

enum class EventLogCategory { Keyboard, Mouse, Window, Touch };

struct EventLogEntry {
    EventLogCategory category;
    std::string line;
};

// ---------------------------------------------------------------------------
// EventsLayer — captures every event and exposes stats for the UI
// ---------------------------------------------------------------------------
class EventsLayer : public vne::testbed::ILayer {
   public:
    static constexpr std::size_t kMaxLog = 20u;

    EventsLayer()
        : vne::testbed::ILayer("EventsLayer") {}

    void onAttach(vne::testbed::AppContext& /*ctx*/) override {}

    void onDetach() override { log_.clear(); }

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

        EventLogCategory cat = EventLogCategory::Keyboard;
        std::string line;
        using ET = vne::events::EventType;
        switch (event.type()) {
            case ET::eKeyPressed: {
                cat = EventLogCategory::Keyboard;
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::Pressed);
                line = "KeyPressed    key=" + std::to_string(static_cast<int>(e.keyCode()));
                break;
            }
            case ET::eKeyRepeat: {
                cat = EventLogCategory::Keyboard;
                const auto& e = static_cast<const vne::events::KeyRepeatEvent&>(event);
                setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::Repeat);
                line = "KeyRepeat     key=" + std::to_string(static_cast<int>(e.keyCode()))
                       + "  count=" + std::to_string(e.repeatCount());
                break;
            }
            case ET::eKeyReleased: {
                cat = EventLogCategory::Keyboard;
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                setLastKey(static_cast<int>(e.keyCode()), LastKeyAction::Released);
                line = "KeyReleased   key=" + std::to_string(static_cast<int>(e.keyCode()));
                break;
            }
            case ET::eMouseButtonPressed: {
                cat = EventLogCategory::Mouse;
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                line = "MousePressed  btn=" + std::to_string(static_cast<int>(e.button()));
                break;
            }
            case ET::eMouseButtonReleased: {
                cat = EventLogCategory::Mouse;
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                line = "MouseReleased btn=" + std::to_string(static_cast<int>(e.button()));
                break;
            }
            case ET::eMouseMoved: {
                cat = EventLogCategory::Mouse;
                const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                line = "MouseMoved    x=" + std::to_string(static_cast<int>(e.x()))
                       + "  y=" + std::to_string(static_cast<int>(e.y()));
                break;
            }
            case ET::eMouseScrolled: {
                cat = EventLogCategory::Mouse;
                const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
                line = "MouseScrolled dx=" + std::to_string(static_cast<int>(e.xOffset()))
                       + "  dy=" + std::to_string(static_cast<int>(e.yOffset()));
                break;
            }
            case ET::eWindowResize: {
                cat = EventLogCategory::Window;
                const auto& e = static_cast<const vne::events::WindowResizeEvent&>(event);
                line = "WindowResize  w=" + std::to_string(e.width()) + "  h=" + std::to_string(e.height());
                break;
            }
            case ET::eWindowClose:
                cat = EventLogCategory::Window;
                line = "WindowClose";
                break;
            case ET::eTouchPress: {
                cat = EventLogCategory::Touch;
                const auto& e = static_cast<const vne::events::TouchPressEvent&>(event);
                setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::Press);
                line = "TouchPress   id=" + std::to_string(e.touchId()) + "  x="
                       + std::to_string(static_cast<int>(e.x())) + "  y=" + std::to_string(static_cast<int>(e.y()));
                break;
            }
            case ET::eTouchRelease: {
                cat = EventLogCategory::Touch;
                const auto& e = static_cast<const vne::events::TouchReleaseEvent&>(event);
                setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::Release);
                line = "TouchRelease id=" + std::to_string(e.touchId()) + "  x="
                       + std::to_string(static_cast<int>(e.x())) + "  y=" + std::to_string(static_cast<int>(e.y()));
                break;
            }
            case ET::eTouchMove: {
                cat = EventLogCategory::Touch;
                const auto& e = static_cast<const vne::events::TouchMoveEvent&>(event);
                setLastTouch(e.touchId(), e.x(), e.y(), LastTouchAction::Move);
                line = "TouchMove    id=" + std::to_string(e.touchId()) + "  x="
                       + std::to_string(static_cast<int>(e.x())) + "  y=" + std::to_string(static_cast<int>(e.y()));
                break;
            }
            default:
                line = event.toString();
                break;
        }

        line = "[f-" + std::to_string(frame_) + "] " + line;
        log_.push_back({cat, std::move(line)});
        if (log_.size() > kMaxLog) {
            log_.pop_front();
        }
    }

    // Accessors for the UI layer
    [[nodiscard]] const std::deque<EventLogEntry>& log() const { return log_; }
    [[nodiscard]] uint64_t totalEvents() const { return events_total_; }
    [[nodiscard]] uint32_t eventsPerSecond() const { return events_per_second_; }

    enum class LastKeyAction { None, Pressed, Released, Repeat };
    [[nodiscard]] int lastKeyCode() const { return last_key_code_; }
    [[nodiscard]] LastKeyAction lastKeyAction() const { return last_key_action_; }

    enum class LastTouchAction { None, Press, Move, Release };
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

    std::deque<EventLogEntry> log_;
    int last_key_code_{-1};
    LastKeyAction last_key_action_{LastKeyAction::None};
    uint32_t last_touch_id_{0};
    double last_touch_x_{0.0};
    double last_touch_y_{0.0};
    LastTouchAction last_touch_action_{LastTouchAction::None};
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
        ImVec4 col = held ? ImVec4(0.9f, 0.3f, 0.15f, 1.f) : ImVec4(0.5f, 0.5f, 0.5f, 1.f);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", label);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(col, "%s", state);
    }

    void renderPanel() {
        // ---- Event log (keyboard / mouse / window with separators and color scheme) ----
        if (ImGui::CollapsingHeader("Events (last 20)", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (events_layer_) {
                ImGui::Text("Total: %llu  /sec: %u",
                            static_cast<unsigned long long>(events_layer_->totalEvents()),
                            events_layer_->eventsPerSecond());
                ImGui::Separator();

                const float log_height = ImGui::GetTextLineHeightWithSpacing() * 8.0f;
                ImGui::BeginChild("EventLog", ImVec2(0.f, log_height), true);
                const auto& log = events_layer_->log();
                for (auto it = log.rbegin(); it != log.rend(); ++it) {
                    ImGui::TextUnformatted(it->line.c_str());
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
                const char* active_name = (hovered >= 0) ? imgui_layer_->getViewportName(hovered) : "—";
                ImGui::Text("Active viewport: %s", active_name);
            }
        }

        // ---- Input polling ----
        if (ImGui::CollapsingHeader("Input Polling", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Keyboard: key table + key press/release display + char entry
            ImGui::Text("Keyboard");
            ImGui::Separator();
            if (ImGui::BeginTable("PollTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 80.f);
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
                const char* action_str = (ka == EventsLayer::LastKeyAction::Pressed)    ? "pressed"
                                         : (ka == EventsLayer::LastKeyAction::Released) ? "released"
                                         : (ka == EventsLayer::LastKeyAction::Repeat)   ? "repeat"
                                                                                        : "";
                const char* key_label = keyCodeToLabel(kc);
                if (action_str[0] != '\0' && kc >= 0) {
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
            ImGui::Text("Last 10: %s", len > 0u ? last_n : "(none)");

            // Mouse: collapsing header
            if (ImGui::CollapsingHeader("Mouse", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto [mx, my] = vne::events::InputManager::mousePosition();
                auto [sx, sy] = vne::events::InputManager::mouseScroll();
                ImGui::Text("Mouse pos:    %d, %d", mx, my);
                ImGui::Text("Mouse scroll: X %.2f  Y %.2f", static_cast<double>(sx), static_cast<double>(sy));
                const char* lb = vne::events::InputManager::isMouseButtonPressed(0) ? "down" : "up  ";
                const char* rb = vne::events::InputManager::isMouseButtonPressed(1) ? "down" : "up  ";
                const char* mb = vne::events::InputManager::isMouseButtonPressed(2) ? "down" : "up  ";
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
                    const char* action_str = (ta == EventsLayer::LastTouchAction::Press)     ? "press"
                                             : (ta == EventsLayer::LastTouchAction::Release) ? "release"
                                             : (ta == EventsLayer::LastTouchAction::Move)    ? "move"
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

void RegisterTestEventsDemo(vne::testbed::Application& app) {
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

VNETESTBED_REGISTER_DEMO("test_events", RegisterTestEventsDemo)
