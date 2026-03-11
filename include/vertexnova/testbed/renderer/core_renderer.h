#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * CoreRenderer: extensible registry of renderers by sort key and name.
 * Application registers MeshRenderer (0), DebugRenderer (100); init runs in sort order.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/renderer/irenderer.h"
#include "vertexnova/testbed/debug_draw.h"

#include <memory>
#include <string>
#include <vector>

namespace vne {
namespace testbed {

class MeshRenderer;

/**
 * @class CoreRenderer
 * @brief Holds renderers by sort key and name; init in sort order; getMeshRenderer, getDebugDraw, getRendererByName.
 */
class CoreRenderer {
   public:
    CoreRenderer() = default;
    ~CoreRenderer();

    CoreRenderer(const CoreRenderer&) = delete;
    CoreRenderer& operator=(const CoreRenderer&) = delete;
    CoreRenderer(CoreRenderer&&) = delete;
    CoreRenderer& operator=(CoreRenderer&&) = delete;

    /**
     * @brief Register a renderer. Init runs in sort-key order.
     * @param renderer Owning pointer (CoreRenderer takes ownership).
     * @param sort_key Lower keys are inited first.
     * @param name     Optional name for getRendererByName().
     */
    void registerRenderer(std::unique_ptr<IRenderer> renderer, int sort_key, std::string name);

    /**
     * @brief Initialize all registered renderers in sort-key order.
     * @return false if any renderer's init failed.
     */
    [[nodiscard]] bool init(IRenderDevice* device);

    /** @brief Shutdown all renderers (reverse order). */
    void shutdown();

    [[nodiscard]] IRenderer* getRendererByName(const std::string& name) const;
    [[nodiscard]] MeshRenderer* getMeshRenderer() const;
    [[nodiscard]] IDebugDraw* getDebugDraw() const;

   private:
    struct Entry {
        int sort_key{0};
        std::string name;
        std::unique_ptr<IRenderer> renderer;
    };
    std::vector<Entry> entries_;
    bool initialized_{false};
};

}  // namespace testbed
}  // namespace vne
