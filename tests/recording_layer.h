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

/**
 * @file recording_layer.h
 * @brief Shared test helper: RecordingLayer records every ILayer lifecycle call.
 *
 * Include this header in any test .cpp that needs a concrete ILayer.
 * total_detach_count uses C++17 inline static so the definition is shared
 * across all translation units without an ODR violation.
 *
 * Usage:
 *   - Check per-instance counters (attach_count, enable_count, ...) for
 *     normal lifecycle assertions.
 *   - Set call_log to a local std::vector<std::string>* to capture ordered
 *     call traces.  Null the pointer again before the local vector goes out
 *     of scope (e.g. before assertions that may early-return on failure).
 */

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include <string>
#include <vector>

struct RecordingLayer : public vne::testbed::ILayer {
    // Per-instance counters
    int attach_count{0};
    int detach_count{0};
    int enable_count{0};
    int disable_count{0};
    int update_count{0};
    int begin_render_count{0};
    int render_count{0};
    int gui_begin_count{0};
    int gui_render_count{0};
    int gui_end_count{0};
    float last_dt{0.0F};

    // Shared across all instances; reset in SetUp() for tests that call clear()
    // and cannot keep raw pointers alive long enough.
    inline static int total_detach_count{0};

    // Optional ordered trace — see file-level note on lifetime
    std::vector<std::string>* call_log{nullptr};

    explicit RecordingLayer(const std::string& name = "RecordingLayer")
        : ILayer(name) {}

    void onAttach(vne::testbed::AppContext& /*ctx*/) override { ++attach_count; }
    void onDetach() override {
        ++detach_count;
        ++total_detach_count;
        if (call_log) {
            call_log->push_back(getName() + "::onDetach");
        }
    }
    void onEnable() override { ++enable_count; }
    void onDisable() override { ++disable_count; }
    void onUpdate(float dt) override {
        ++update_count;
        last_dt = dt;
    }
    void onBeginRender(const vne::testbed::RenderContext& /*ctx*/) override { ++begin_render_count; }
    void onRender(const vne::testbed::RenderContext& /*ctx*/) override { ++render_count; }
    void onGuiBegin(const vne::testbed::RenderContext& /*ctx*/) override { ++gui_begin_count; }
    void onGuiRender(const vne::testbed::RenderContext& /*ctx*/) override { ++gui_render_count; }
    void onGuiEnd(const vne::testbed::RenderContext& /*ctx*/) override {
        ++gui_end_count;
        if (call_log) {
            call_log->push_back(getName() + "::onGuiEnd");
        }
    }
};
