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

#include "vertexnova/testbed/renderer/core_renderer.h"

#include "vertexnova/testbed/renderer/debug_renderer.h"
#include "vertexnova/testbed/renderer/mesh_renderer.h"
#include "vertexnova/testbed/render_device.h"

#ifdef VNE_TESTBED_LOGGING
#include "vertexnova/logging/logging.h"
#endif

#include <algorithm>

namespace vne {
namespace testbed {

#ifdef VNE_TESTBED_LOGGING
namespace {
CREATE_VNE_LOGGER_CATEGORY("vnetestbed.renderer.core")
}  // namespace
#endif

CoreRenderer::~CoreRenderer() {
    shutdown();
}

void CoreRenderer::registerRenderer(std::unique_ptr<IRenderer> renderer, int sort_key, std::string name) {
    if (!renderer) {
        return;
    }
    entries_.push_back({sort_key, std::move(name), std::move(renderer)});
}

bool CoreRenderer::init(IRenderDevice* device) {
    if (!device || initialized_) {
        return false;
    }
    std::stable_sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
        return a.sort_key < b.sort_key;
    });
#ifdef VNE_TESTBED_LOGGING
    VNE_LOG_DEBUG << "CoreRenderer: init " << entries_.size() << " renderers";
#endif
    for (auto& e : entries_) {
        if (!e.renderer->init(device)) {
#ifdef VNE_TESTBED_LOGGING
            VNE_LOG_ERROR << "CoreRenderer: init failed for '" << e.name << "'";
#endif
            return false;
        }
#ifdef VNE_TESTBED_LOGGING
        VNE_LOG_TRACE << "CoreRenderer: inited '" << e.name << "' (key " << e.sort_key << ")";
#endif
    }
    initialized_ = true;
    return true;
}

void CoreRenderer::shutdown() {
    if (!initialized_) {
        return;
    }
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        it->renderer->shutdown();
    }
    initialized_ = false;
}

void CoreRenderer::resize(int width, int height) {
    for (auto& e : entries_) {
        if (e.renderer) {
            e.renderer->resize(width, height);
        }
    }
}

IRenderer* CoreRenderer::getRendererByName(const std::string& name) const {
    for (const auto& e : entries_) {
        if (e.name == name) {
            return e.renderer.get();
        }
    }
    return nullptr;
}

MeshRenderer* CoreRenderer::getMeshRenderer() const {
    return dynamic_cast<MeshRenderer*>(getRendererByName("mesh"));
}

IDebugDraw* CoreRenderer::getDebugDraw() const {
    return dynamic_cast<IDebugDraw*>(getRendererByName("debug"));
}

}  // namespace testbed
}  // namespace vne
