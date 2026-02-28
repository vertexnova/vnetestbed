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
 * window/renderer/debugDraw.  Per frame the runner calls the stack methods in
 * its preferred order (typical: onUpdate, onGuiBegin, onGuiRender, onGuiEnd,
 * onBeginRender, onRender).  onGuiEnd fires in full reverse of onGuiBegin
 * (last begun is first ended — overlays reversed, then layers reversed).
 * onEvent propagates top-to-bottom (overlays first, then layers, both in
 * reverse push order); a layer can stop propagation by returning blocksInput().
 * onDetach is called on pop or clear.
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

    // -----------------------------------------------------------------------
    // Lifecycle hooks — called by LayerStack
    // -----------------------------------------------------------------------

    /** @brief Called once when the layer is pushed onto the stack. */
    virtual void onAttach(AppContext& ctx) { (void)ctx; }

    /** @brief Called once when the layer is popped or the stack is cleared. */
    virtual void onDetach() {}

    /** @brief Called when the layer transitions from disabled to enabled. */
    virtual void onEnable() {}

    /** @brief Called when the layer transitions from enabled to disabled. */
    virtual void onDisable() {}

    // -----------------------------------------------------------------------
    // Per-frame hooks
    // -----------------------------------------------------------------------

    /** @brief Logic update. @param dt Delta time in seconds since last frame. */
    virtual void onUpdate(float dt) { (void)dt; }

    /** @brief Pre-render setup (clear buffers, begin passes, etc.). */
    virtual void onBeginRender(const RenderContext& ctx) { (void)ctx; }

    /** @brief Scene rendering; only called when isEnabled() && isVisible(). */
    virtual void onRender(const RenderContext& ctx) { (void)ctx; }

    /** @brief Begin immediate-mode GUI frame. */
    virtual void onGuiBegin(const RenderContext& ctx) { (void)ctx; }

    /** @brief Emit GUI widgets. */
    virtual void onGuiRender(const RenderContext& ctx) { (void)ctx; }

    /**
     * @brief End immediate-mode GUI frame.
     *
     * Called in reverse of onGuiBegin order so that begin/end pairs nest
     * correctly (last layer begun is first layer ended).
     */
    virtual void onGuiEnd(const RenderContext& ctx) { (void)ctx; }

    // -----------------------------------------------------------------------
    // Event hook
    // -----------------------------------------------------------------------

    /**
     * @brief Handle an input or system event.
     *
     * Propagation stops at the first layer where blocksInput() returns true.
     * Called top-to-bottom (overlays first, then layers, both in reverse push
     * order) so that the topmost visual element gets first refusal.
     */
    virtual void onEvent(const vne::events::Event& event) { (void)event; }

    // -----------------------------------------------------------------------
    // State accessors
    // -----------------------------------------------------------------------

    /** @brief Human-readable name used for debugging and lookup. */
    [[nodiscard]] const std::string& getName() const { return name_; }

    /**
     * @brief Whether the layer receives per-frame callbacks.
     *
     * Use setEnabled() to transition; it fires onEnable()/onDisable()
     * automatically on state change.
     */
    [[nodiscard]] bool isEnabled() const { return is_enabled_; }

    /**
     * @brief Enable or disable the layer.
     *
     * Fires onEnable() when transitioning false→true, onDisable() when
     * transitioning true→false.  No-op if the state is unchanged.
     */
    void setEnabled(bool enabled);

    /** @brief Whether onRender() is called (does not suppress GUI or update). */
    [[nodiscard]] bool isVisible() const { return is_visible_; }

    /** @brief Show or hide the layer's rendered output. */
    void setVisible(bool visible) { is_visible_ = visible; }

    /** @brief Whether this layer receives onEvent() calls. */
    [[nodiscard]] bool wantsInput() const { return wants_input_; }

    /** @brief Enable or disable event delivery to this layer. */
    void setWantsInput(bool wants) { wants_input_ = wants; }

    /** @brief Whether this layer stops event propagation after onEvent(). */
    [[nodiscard]] bool blocksInput() const { return blocks_input_; }

    /** @brief Set whether this layer stops event propagation. */
    void setBlocksInput(bool blocks) { blocks_input_ = blocks; }

    /** @brief Sort key for custom render ordering (lower values drawn first). */
    [[nodiscard]] int getRenderSortKey() const { return render_sort_key_; }

    /** @brief Override the render sort key. */
    void setRenderSortKey(int key) { render_sort_key_ = key; }

   protected:
    bool is_enabled_{true};
    bool is_visible_{true};
    bool wants_input_{true};
    bool blocks_input_{false};
    int render_sort_key_{0};

   private:
    std::string name_;
};

}  // namespace testbed
}  // namespace vne
