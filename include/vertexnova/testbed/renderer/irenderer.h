#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Base interface for scene renderers (mesh, debug, volume, slice, etc.).
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/render_context.h"

namespace vne {
namespace testbed {

class IRenderDevice;

/**
 * @class IRenderer
 * @brief Common base for all scene renderers so CoreRenderer can hold and init them uniformly.
 *
 * Implementations: MeshRenderer, DebugRenderer, and future VolumeRenderer, SliceRenderer.
 */
class IRenderer {
   public:
    virtual ~IRenderer() = default;

    IRenderer(const IRenderer&) = delete;
    IRenderer& operator=(const IRenderer&) = delete;
    IRenderer(IRenderer&&) = delete;
    IRenderer& operator=(IRenderer&&) = delete;

    /**
     * @brief Initialize GPU resources. Call after a valid device exists.
     * @param device Backend-agnostic render device.
     * @return true on success.
     */
    [[nodiscard]] virtual bool init(IRenderDevice* device) = 0;

    /** @brief Release all GPU resources. */
    virtual void shutdown() = 0;

    /**
     * @brief Called when the framebuffer (window or main render target) is resized.
     * Update viewport-dependent resources (e.g. resolution-dependent buffers) here.
     */
    virtual void resize(int width, int height) {
        (void)width;
        (void)height;
    }

    /**
     * @brief Optional: render pass. For a future unified loop, CoreRenderer can call this in sort order.
     * Default no-op so existing flow (layers call draw methods directly) is unchanged.
     */
    virtual void render(const RenderContext& ctx) { (void)ctx; }

   protected:
    IRenderer() = default;
};

}  // namespace testbed
}  // namespace vne
