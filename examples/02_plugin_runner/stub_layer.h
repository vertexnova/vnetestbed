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

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

namespace vne {
namespace testbed {

/** Stub layer to validate LayerStack lifecycle. */
struct StubLayer : ILayer {
    StubLayer()
        : ILayer("StubLayer") {}

    void onAttach(AppContext& /*ctx*/) override {}
    void onDetach() override {}
    void onUpdate(float /*dt*/) override {}
    void onBeginRender(const RenderContext& /*ctx*/) override {}
    void onRender(const RenderContext& /*ctx*/) override {}
    void onGuiBegin(const RenderContext& /*ctx*/) override {}
    void onGuiRender(const RenderContext& /*ctx*/) override {}
    void onGuiEnd(const RenderContext& /*ctx*/) override {}
};

}  // namespace testbed
}  // namespace vne
