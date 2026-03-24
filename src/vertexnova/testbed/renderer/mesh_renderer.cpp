/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * AutoDoc:   Yes
 *
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/renderer/mesh_renderer.h"

#include "vertexnova/testbed/render_device.h"
#include "vertexnova/scene/camera/camera.h"

#ifdef VNE_TESTBED_LOGGING
#include "vertexnova/logging/logging.h"
#endif

namespace vne {
namespace testbed {

#ifdef VNE_TESTBED_LOGGING
namespace {
CREATE_VNE_LOGGER_CATEGORY("vnetestbed.renderer.mesh")
}  // namespace
#endif

MeshRenderer::MeshRenderer() = default;

MeshRenderer::~MeshRenderer() {
    shutdown();
}

bool MeshRenderer::init(IRenderDevice* device) {
    if (!device) {
        return false;
    }
    device_ = device;
    if (!phong_material_.init(device)) {
#ifdef VNE_TESTBED_LOGGING
        VNE_LOG_ERROR << "MeshRenderer: PhongMaterial init failed";
#endif
        return false;
    }
#ifdef VNE_TESTBED_LOGGING
    VNE_LOG_DEBUG << "MeshRenderer: init OK";
#endif
    return true;
}

void MeshRenderer::shutdown() {
    phong_material_.shutdown();
    device_ = nullptr;
    framebuffer_width_ = 0;
    framebuffer_height_ = 0;
}

void MeshRenderer::resize(int width, int height) {
    framebuffer_width_ = width;
    framebuffer_height_ = height;
#ifdef VNE_TESTBED_LOGGING
    VNE_LOG_TRACE << "MeshRenderer: resize " << framebuffer_width_ << "x" << framebuffer_height_;
#endif
}

void MeshRenderer::drawMesh(IRenderDevice* device,
                            vne::scene::ICamera* camera,
                            BufferHandle vbo,
                            BufferHandle ibo,
                            uint32_t index_count,
                            const vne::math::Mat4f& model,
                            const PhongLightParams& lights) {
    if (!device || !camera || !phong_material_.isReady() || !vbo.isValid() || !ibo.isValid() || index_count == 0) {
        return;
    }
    const vne::math::Mat4f vp = camera->getViewProjectionMatrix();
    const vne::math::Mat4f mvp = vp * model;
    const vne::math::Vec3f cam_pos = camera->getPosition();
    phong_material_.setUniforms(device, mvp, model, cam_pos, lights);
    device->drawIndexed(phong_material_.getPipeline(), vbo, ibo, index_count, DrawMode::eTriangles);
}

}  // namespace testbed
}  // namespace vne
