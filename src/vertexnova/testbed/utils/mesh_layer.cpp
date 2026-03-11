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

#include "vertexnova/logging/logging.h"
#include "vertexnova/math/math.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#ifdef VNE_TESTBED_HAVE_VNEIO
#include "vertexnova/io/mesh/assimp_loader.h"
#include "vertexnova/io/mesh/mesh.h"
#endif

#include <cmath>
#include <vector>

namespace vne {
namespace testbed {

namespace {
#ifdef VNE_TESTBED_HAVE_VNEIO
CREATE_VNE_LOGGER_CATEGORY("vnetestbed.utils.mesh_layer")
#endif
}  // namespace

MeshLayer::MeshLayer()
    : ILayer("MeshLayer") {}

void MeshLayer::setMeshPath(std::string path) {
    mesh_path_ = std::move(path);
}

void MeshLayer::setCameraProvider(CameraProvider provider) {
    camera_provider_ = std::move(provider);
}

void MeshLayer::setShaderPaths(std::filesystem::path vert_path, std::filesystem::path frag_path) {
    vert_path_ = std::move(vert_path);
    frag_path_ = std::move(frag_path);
}

void MeshLayer::onAttach(AppContext& ctx) {
    device_ = ctx.device;
    if (!device_) {
        return;
    }
#ifdef VNE_TESTBED_HAVE_VNEIO
    if (mesh_path_.empty() || vert_path_.empty() || frag_path_.empty()) {
        return;
    }
    vne::mesh::AssimpLoader loader;
    vne::mesh::AssimpLoaderOptions opts;
    opts.calc_normals_if_missing = true;
    vne::mesh::Mesh mesh;
    if (!loader.loadFile(mesh_path_, mesh, opts)) {
        VNE_LOG_ERROR << "MeshLayer: failed to load " << mesh_path_ << ": " << loader.getLastError();
        return;
    }
    if (mesh.isEmpty()) {
        VNE_LOG_ERROR << "MeshLayer: mesh is empty " << mesh_path_;
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

    shader_ = device_->createShader(vert_path_, frag_path_);
    if (!shader_.isValid()) {
        VNE_LOG_ERROR << "MeshLayer: shaders not loaded (ensure vert/frag paths are valid).";
        return;
    }
    PipelineDesc pd{};
    pd.shader = shader_;
    pd.layout = {{3}, {3}, {3}};
    pd.depth.testEnabled = true;
    pd.depth.writeEnabled = true;
    pd.rasterizer.cull = CullMode::eBack;
    pipeline_ = device_->createPipeline(pd);
    ready_ = vbo_.isValid() && ibo_.isValid() && pipeline_.isValid();
#else
    (void)vert_path_;
    (void)frag_path_;
#endif
}

void MeshLayer::onDetach() {
    if (device_) {
        if (pipeline_.isValid())
            device_->destroy(pipeline_);
        if (ibo_.isValid())
            device_->destroy(ibo_);
        if (vbo_.isValid())
            device_->destroy(vbo_);
        if (shader_.isValid())
            device_->destroy(shader_);
    }
    pipeline_ = {};
    ibo_ = {};
    vbo_ = {};
    shader_ = {};
    device_ = nullptr;
    index_count_ = 0;
    ready_ = false;
}

void MeshLayer::onRender(const RenderContext& ctx) {
    if (!ready_ || !device_ || !camera_provider_) {
        return;
    }
    const int vp_idx = (ctx.active_viewport_index >= 0) ? ctx.active_viewport_index : 0;
    std::shared_ptr<vne::scene::ICamera> camera = camera_provider_(vp_idx);
    if (!camera) {
        return;
    }
    if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
        if (auto* persp = dynamic_cast<vne::scene::PerspectiveCamera*>(camera.get())) {
            persp->setAspectRatio(static_cast<float>(ctx.frame_info.width)
                                  / static_cast<float>(ctx.frame_info.height));
        }
        camera->updateProjectionMatrix();
    }
    const vne::math::Mat4f model = vne::math::Mat4f::identity();
    const vne::math::Mat4f vp = camera->getViewProjectionMatrix();
    const vne::math::Mat4f mvp = vp * model;

    device_->setMat4(shader_, "u_mvp", mvp);
    device_->setMat4(shader_, "u_model", model);
    device_->setVec3(shader_, "u_camPos", camera->getPosition());
    device_->setVec3(shader_, "u_ambientColor", vne::math::Vec3f{0.4f, 0.4f, 0.45f});
    device_->setFloat(shader_, "u_ambientIntensity", 1.0f);
    device_->setInt(shader_, "u_dirLightEnabled", 0);
    device_->setInt(shader_, "u_numPointLights", 0);
    device_->setInt(shader_, "u_spotLightEnabled", 0);
    device_->setFloat(shader_, "u_attnConst", 1.0f);
    device_->setFloat(shader_, "u_attnLinear", 0.09f);
    device_->setFloat(shader_, "u_attnQuad", 0.032f);
    device_->setInt(shader_, "u_useAttnFormula", 0);

    device_->drawIndexed(pipeline_, vbo_, ibo_, index_count_, DrawMode::eTriangles);
}

}  // namespace testbed
}  // namespace vne
