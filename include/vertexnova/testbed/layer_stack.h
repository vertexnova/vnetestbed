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
 * @file layer_stack.h
 * @brief Owns and drives the lifecycle of layers and overlays.
 *
 * Architecture matches samples/core LayerStack_C; naming follows CODING_GUIDELINES.
 * Layers and overlays are stored separately; overlays render on top.
 */

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include <memory>
#include <string>
#include <vector>

namespace vne {
namespace events {
class Event;
}
}  // namespace vne

namespace vne {
namespace testbed {

/**
 * @class LayerStack
 * @brief Manages layers and overlays; dispatches lifecycle and render callbacks.
 */
class LayerStack {
   public:
    LayerStack() = default;
    ~LayerStack();

    LayerStack(const LayerStack&) = delete;
    LayerStack& operator=(const LayerStack&) = delete;
    LayerStack(LayerStack&&) noexcept = default;
    LayerStack& operator=(LayerStack&&) noexcept = default;

    void pushLayer(std::unique_ptr<ILayer> layer, AppContext& ctx);
    void pushOverlay(std::unique_ptr<ILayer> overlay, AppContext& ctx);
    std::unique_ptr<ILayer> popLayer();
    std::unique_ptr<ILayer> popOverlay();
    void clear();

    void onUpdate(float dt);
    void onBeginRender(const RenderContext& ctx);
    void onRender(const RenderContext& ctx);
    /** @brief Render only layers (not overlays); used when ImGuiLayer does per-viewport render. */
    void onRenderLayersOnly(const RenderContext& ctx);
    void onGuiBegin(const RenderContext& ctx);
    void onGuiRender(const RenderContext& ctx);
    void onGuiEnd(const RenderContext& ctx);
    void onEvent(const vne::events::Event& event);

    [[nodiscard]] std::size_t getCount() const { return layers_.size(); }
    [[nodiscard]] std::size_t getOverlayCount() const { return overlays_.size(); }
    [[nodiscard]] ILayer* findLayerByName(const std::string& name) const;
    [[nodiscard]] std::vector<ILayer*> getAll() const;

   private:
    std::vector<std::unique_ptr<ILayer>> layers_;
    std::vector<std::unique_ptr<ILayer>> overlays_;
};

}  // namespace testbed
}  // namespace vne
