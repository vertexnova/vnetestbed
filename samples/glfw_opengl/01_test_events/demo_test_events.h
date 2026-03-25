#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include "vertexnova/events/event.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace vne::testbed {
class Application;
}
#ifdef VNE_TESTBED_IMGUI
namespace vne::testbed {
class ImGuiLayer;
}
#endif

namespace vne::samples::test_events {

enum class LastKeyAction { eNone = 0, ePressed = 1, eReleased = 2, eRepeat = 3 };

enum class LastTouchAction { eNone = 0, ePress = 1, eMove = 2, eRelease = 3 };

// ---------------------------------------------------------------------------
// EventsLayer — captures every event and exposes stats for the UI
// ---------------------------------------------------------------------------
class EventsLayer : public vne::testbed::ILayer {
   public:
    static constexpr std::size_t kMaxLog = 20;
    static constexpr int kInvalidKeyCode = -1;

    EventsLayer();

    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onEvent(const vne::events::Event& event) override;

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
    void setLastKey(int key_code, LastKeyAction action);
    void setLastTouch(uint32_t id, double x, double y, LastTouchAction action);

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
    EventsSettingsLayer();

    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
    void setEventsLayer(EventsLayer* layer);

    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;

   private:
    static const char* keyCodeToLabel(int key_code);
    static void keyPollRow(const char* label, int key);
    void renderPanel();

    static constexpr size_t kCharDisplayBufSize = 256;
    static constexpr size_t kCharDisplayLastN = 10;
    char char_display_buf_[kCharDisplayBufSize] = {};

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    EventsLayer* events_layer_{nullptr};
};
#endif  // VNE_TESTBED_IMGUI

void registerTestEventsDemo(vne::testbed::Application& app);

}  // namespace vne::samples::test_events
