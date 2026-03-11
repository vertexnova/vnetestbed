/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * MeshLayer implementation. Mesh loading uses vneio when VNE_TESTBED_HAVE_VNEIO.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/utils/mesh_layer.h"

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/renderer/core_renderer.h"
#include "vertexnova/testbed/renderer/mesh_renderer.h"
#include "vertexnova/testbed/renderer/phong_material.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/math/math.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#ifdef VNE_TESTBED_HAVE_VNEIO
#include "vertexnova/io/mesh/assimp_loader.h"
#include "vertexnova/io/mesh/mesh.h"
#endif

#include <vector>

namespace vne {
namespace testbed {

#ifdef VNE_TESTBED_LOGGING
namespace {
CREATE_VNE_LOGGER_CATEGORY("vnetestbed.utils.mesh_layer")
}  // namespace
#endif

MeshLayer::MeshLayer()
    : ILayer("MeshLayer") {}

void MeshLayer::setMeshPath(std::string path) {
    mesh_path_ = std::move(path);
}

void MeshLayer::setCameraProvider(CameraProvider provider) {
    camera_provider_ = std::move(provider);
}

void MeshLayer::onAttach(AppContext& ctx) {
    device_ = ctx.device;
    if (!device_) {
        return;
    }
    if (!ctx.coreRenderer) {
#ifdef VNE_TESTBED_LOGGING
        VNE_LOG_ERROR << "MeshLayer: AppContext has no coreRenderer";
#endif
        return;
    }
    mesh_renderer_ = ctx.coreRenderer->getMeshRenderer();
    if (!mesh_renderer_) {
#ifdef VNE_TESTBED_LOGGING
        VNE_LOG_ERROR << "MeshLayer: coreRenderer has no MeshRenderer";
#endif
        return;
    }
    loadMeshFromPath();
}

void MeshLayer::reloadMesh(std::string path) {
    if (!device_) {
        mesh_path_ = std::move(path);
        return;
    }
    if (ibo_.isValid()) {
        device_->destroy(ibo_);
    }
    if (vbo_.isValid()) {
        device_->destroy(vbo_);
    }
    ibo_ = {};
    vbo_ = {};
    index_count_ = 0;
    ready_ = false;
    mesh_path_ = std::move(path);
    loadMeshFromPath();
}

void MeshLayer::loadMeshFromPath() {
#ifdef VNE_TESTBED_HAVE_VNEIO
    if (mesh_path_.empty() || !device_) {
        return;
    }
    vne::mesh::AssimpLoader loader;
    vne::mesh::AssimpLoaderOptions opts;
    opts.calc_normals_if_missing = true;
    vne::mesh::Mesh mesh;
    if (!loader.loadFile(mesh_path_, mesh, opts)) {
#ifdef VNE_TESTBED_LOGGING
        VNE_LOG_ERROR << "MeshLayer: failed to load " << mesh_path_ << ": " << loader.getLastError();
#endif
        return;
    }
    if (mesh.isEmpty()) {
#ifdef VNE_TESTBED_LOGGING
        VNE_LOG_ERROR << "MeshLayer: mesh is empty " << mesh_path_;
#endif
        return;
    }
    const size_t vertex_count = mesh.getVertexCount();
    const size_t index_count = mesh.getIndexCount();
    std::vector<float> vbo_data;
    vbo_data.reserve(vertex_count * 9u);
    const float default_color[3] = {0.6f, 0.6f, 0.65f};
    for (size_t i = 0; i < vertex_count; ++i) {
        const auto& v = mesh.vertices[i];
        vbo_data.push_back(v.position[0]);
        vbo_data.push_back(v.position[1]);
        vbo_data.push_back(v.position[2]);
        vbo_data.push_back(v.normal[0]);
        vbo_data.push_back(v.normal[1]);
        vbo_data.push_back(v.normal[2]);
        vbo_data.push_back(default_color[0]);
        vbo_data.push_back(default_color[1]);
        vbo_data.push_back(default_color[2]);
    }
    vbo_ = device_->createVertexBuffer(vbo_data.data(), static_cast<uint32_t>(vbo_data.size() * sizeof(float)));
    ibo_ = device_->createIndexBuffer(mesh.indices.data(), static_cast<uint32_t>(index_count));
    index_count_ = static_cast<uint32_t>(index_count);
    ready_ = vbo_.isValid() && ibo_.isValid();
#ifdef VNE_TESTBED_LOGGING
    if (ready_) {
        VNE_LOG_INFO << "MeshLayer: loaded " << mesh_path_ << " (" << vertex_count << " verts, " << index_count
                     << " idx)";
    }
#endif
#endif
}

void MeshLayer::onDetach() {
    if (device_) {
        if (ibo_.isValid())
            device_->destroy(ibo_);
        if (vbo_.isValid())
            device_->destroy(vbo_);
    }
    ibo_ = {};
    vbo_ = {};
    device_ = nullptr;
    mesh_renderer_ = nullptr;
    index_count_ = 0;
    ready_ = false;
}

void MeshLayer::onRender(const RenderContext& ctx) {
    if (!ready_ || !device_ || !mesh_renderer_ || !camera_provider_) {
        return;
    }
    const int vp_idx = (ctx.active_viewport_index >= 0) ? ctx.active_viewport_index : 0;
    std::shared_ptr<vne::scene::ICamera> camera = camera_provider_(vp_idx);
    if (!camera) {
        return;
    }
    if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
        if (auto* persp = dynamic_cast<vne::scene::PerspectiveCamera*>(camera.get())) {
            persp->setAspectRatio(static_cast<float>(ctx.frame_info.width) / static_cast<float>(ctx.frame_info.height));
        }
        camera->updateProjectionMatrix();
    }
    const vne::math::Mat4f model = vne::math::Mat4f::identity();
    PhongLightParams lights{};  // default ambient only
    mesh_renderer_->drawMesh(device_, camera.get(), vbo_, ibo_, index_count_, model, lights);
}

}  // namespace testbed
}  // namespace vne
