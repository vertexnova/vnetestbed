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

/**
 * @file layer.h
 * @brief Layer interface aligned with samples/core Layer_I.
 *
 * LayerStack invokes: onAttach(ctx) once with AppContext so layers can cache
 * window/renderer/debugDraw; then each frame onUpdate -> onBeginRender ->
 * onRender -> onGuiBegin -> onGuiRender -> onGuiEnd; onDetach on exit.
 * onEvent when events occur.
 */

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/render_context.h"

#include <string>

namespace vne {
namespace events {
class Event;
}
}  // namespace vne

namespace vne {
namespace testbed {

/**
 * @class ILayer
 * @brief Lifecycle interface every testbed layer must implement.
 *
 * Layers are owned by LayerStack, which calls each hook in order.
 * Architecture matches samples/core; naming follows CODING_GUIDELINES.
 */
class ILayer {
   public:
    explicit ILayer(const std::string& name = "Layer");
    virtual ~ILayer() = default;

    ILayer(const ILayer&) = delete;
    ILayer& operator=(const ILayer&) = delete;
    ILayer(ILayer&&) = delete;
    ILayer& operator=(ILayer&&) = delete;

    // Lifecycle
    virtual void onAttach(AppContext& ctx) { (void)ctx; }
    virtual void onDetach() {}
    virtual void onEnable() {}
    virtual void onDisable() {}

    // Per-frame (context passed)
    virtual void onUpdate(float dt) { (void)dt; }
    virtual void onBeginRender(const RenderContext& ctx) { (void)ctx; }
    virtual void onRender(const RenderContext& ctx) { (void)ctx; }
    virtual void onGuiBegin(const RenderContext& ctx) { (void)ctx; }
    virtual void onGuiRender(const RenderContext& ctx) { (void)ctx; }
    virtual void onGuiEnd(const RenderContext& ctx) { (void)ctx; }

    // Event (uses vneevents)
    virtual void onEvent(const vne::events::Event& event) { (void)event; }

    // State
    const std::string& getName() const { return name_; }
    bool isEnabled() const { return is_enabled_; }
    void setEnabled(bool enabled);
    bool isVisible() const { return is_visible_; }
    void setVisible(bool visible) { is_visible_ = visible; }
    bool wantsInput() const { return wants_input_; }
    void setWantsInput(bool wants) { wants_input_ = wants; }
    bool blocksInput() const { return blocks_input_; }
    void setBlocksInput(bool blocks) { blocks_input_ = blocks; }
    int getRenderSortKey() const { return render_sort_key_; }
    void setRenderSortKey(int key) { render_sort_key_ = key; }

   protected:
    std::string name_;
    bool is_enabled_{true};
    bool is_visible_{true};
    bool wants_input_{true};
    bool blocks_input_{false};
    int render_sort_key_{0};
};

}  // namespace testbed
}  // namespace vne
