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

#include "vertexnova/testbed/layer_stack.h"

#include "vertexnova/events/event.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace vne {
namespace testbed {

LayerStack::~LayerStack() {
    clear();
}

void LayerStack::pushLayer(std::unique_ptr<ILayer> layer, AppContext& ctx) {
    if (!layer) {
        return;
    }
    layer->onAttach(ctx);
    if (layer->isEnabled()) {
        layer->onEnable();
    }
    layers_.push_back(std::move(layer));
}

void LayerStack::pushOverlay(std::unique_ptr<ILayer> overlay, AppContext& ctx) {
    if (!overlay) {
        return;
    }
    overlay->onAttach(ctx);
    // New overlays default to enabled; call onEnable() so enabled state is consistent.
    if (overlay->isEnabled()) {
        overlay->onEnable();
    }
    overlays_.push_back(std::move(overlay));
}

std::unique_ptr<ILayer> LayerStack::popLayer() {
    if (layers_.empty()) {
        return nullptr;
    }
    auto layer = std::move(layers_.back());
    layers_.pop_back();
    layer->setEnabled(false);  // updates is_enabled_ and fires onDisable() if needed
    layer->onDetach();
    return layer;
}

std::unique_ptr<ILayer> LayerStack::popOverlay() {
    if (overlays_.empty()) {
        return nullptr;
    }
    auto overlay = std::move(overlays_.back());
    overlays_.pop_back();
    overlay->setEnabled(false);  // updates is_enabled_ and fires onDisable() if needed
    overlay->onDetach();
    return overlay;
}

void LayerStack::clear() {
    // Tear down in reverse push order: overlays (LIFO) then layers (LIFO).
    // Use setEnabled(false) so is_enabled_ is updated (consistent with popLayer/popOverlay).
    for (auto it = overlays_.rbegin(); it != overlays_.rend(); ++it) {
        if (*it) {
            (*it)->setEnabled(false);
            (*it)->onDetach();
        }
    }
    overlays_.clear();

    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        if (*it) {
            (*it)->setEnabled(false);
            (*it)->onDetach();
        }
    }
    layers_.clear();
}

void LayerStack::onUpdate(float dt) {
    for (const auto& layer : layers_) {
        if (layer && layer->isEnabled()) {
            layer->onUpdate(dt);
        }
    }
    for (const auto& overlay : overlays_) {
        if (overlay && overlay->isEnabled()) {
            overlay->onUpdate(dt);
        }
    }
}

void LayerStack::onBeginRender(const RenderContext& ctx) {
    for (const auto& layer : layers_) {
        if (layer && layer->isEnabled()) {
            layer->onBeginRender(ctx);
        }
    }
    for (const auto& overlay : overlays_) {
        if (overlay && overlay->isEnabled()) {
            overlay->onBeginRender(ctx);
        }
    }
}

void LayerStack::onRender(const RenderContext& ctx) {
    std::vector<std::pair<int, ILayer*>> sorted_layers;
    sorted_layers.reserve(layers_.size());
    for (const auto& layer : layers_) {
        if (layer && layer->isEnabled() && layer->isVisible()) {
            sorted_layers.emplace_back(layer->getRenderSortKey(), layer.get());
        }
    }
    std::stable_sort(sorted_layers.begin(), sorted_layers.end(),
                     [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& p : sorted_layers) {
        if (p.second) {
            p.second->onRender(ctx);
        }
    }
    std::vector<std::pair<int, ILayer*>> sorted_overlays;
    sorted_overlays.reserve(overlays_.size());
    for (const auto& overlay : overlays_) {
        if (overlay && overlay->isEnabled() && overlay->isVisible()) {
            sorted_overlays.emplace_back(overlay->getRenderSortKey(), overlay.get());
        }
    }
    std::stable_sort(sorted_overlays.begin(), sorted_overlays.end(),
                     [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& p : sorted_overlays) {
        if (p.second) {
            p.second->onRender(ctx);
        }
    }
}

void LayerStack::onRenderLayersOnly(const RenderContext& ctx) {
    std::vector<std::pair<int, ILayer*>> sorted_layers;
    sorted_layers.reserve(layers_.size());
    for (const auto& layer : layers_) {
        if (layer && layer->isEnabled() && layer->isVisible()) {
            sorted_layers.emplace_back(layer->getRenderSortKey(), layer.get());
        }
    }
    std::stable_sort(sorted_layers.begin(), sorted_layers.end(),
                     [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& p : sorted_layers) {
        if (p.second) {
            p.second->onRender(ctx);
        }
    }
}

void LayerStack::onGuiBegin(const RenderContext& ctx) {
    for (const auto& layer : layers_) {
        if (layer && layer->isEnabled()) {
            layer->onGuiBegin(ctx);
        }
    }
    for (const auto& overlay : overlays_) {
        if (overlay && overlay->isEnabled()) {
            overlay->onGuiBegin(ctx);
        }
    }
}

void LayerStack::onGuiRender(const RenderContext& ctx) {
    for (const auto& layer : layers_) {
        if (layer && layer->isEnabled()) {
            layer->onGuiRender(ctx);
        }
    }
    for (const auto& overlay : overlays_) {
        if (overlay && overlay->isEnabled()) {
            overlay->onGuiRender(ctx);
        }
    }
}

void LayerStack::onGuiEnd(const RenderContext& ctx) {
    // Full reverse of onGuiBegin: overlays top-to-bottom first, then layers top-to-bottom.
    // Mirrors the begin/end stack discipline (last begun is first ended).
    for (auto it = overlays_.rbegin(); it != overlays_.rend(); ++it) {
        if (*it && (*it)->isEnabled()) {
            (*it)->onGuiEnd(ctx);
        }
    }
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        if (*it && (*it)->isEnabled()) {
            (*it)->onGuiEnd(ctx);
        }
    }
}

void LayerStack::onEvent(const vne::events::Event& event) {
    for (auto it = overlays_.rbegin(); it != overlays_.rend(); ++it) {
        if (*it && (*it)->isEnabled() && (*it)->wantsInput()) {
            (*it)->onEvent(event);
            if ((*it)->blocksInput()) {
                return;
            }
        }
    }
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        if (*it && (*it)->isEnabled() && (*it)->wantsInput()) {
            (*it)->onEvent(event);
            if ((*it)->blocksInput()) {
                return;
            }
        }
    }
}

ILayer* LayerStack::findLayerByName(const std::string& name) const {
    for (const auto& layer : layers_) {
        if (layer && layer->getName() == name) {
            return layer.get();
        }
    }
    for (const auto& overlay : overlays_) {
        if (overlay && overlay->getName() == name) {
            return overlay.get();
        }
    }
    return nullptr;
}

std::vector<ILayer*> LayerStack::getAll() const {
    std::vector<ILayer*> result;
    result.reserve(layers_.size() + overlays_.size());
    for (const auto& layer : layers_) {
        if (layer) {
            result.push_back(layer.get());
        }
    }
    for (const auto& overlay : overlays_) {
        if (overlay) {
            result.push_back(overlay.get());
        }
    }
    return result;
}

}  // namespace testbed
}  // namespace vne
