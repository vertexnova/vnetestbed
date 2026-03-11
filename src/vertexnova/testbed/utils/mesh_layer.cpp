/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * MeshLayer: loads mesh via vneio, converts vnescene ILight objects to
 * PhongLightParams each frame, and draws with MeshRenderer.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/utils/mesh_layer.h"

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/renderer/core_renderer.h"
#include "vertexnova/testbed/renderer/mesh_renderer.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/light/light.h"

#ifdef VNE_TESTBED_VNEIO
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
    : ILayer("MeshLayer") {
    // Default lights: ambient fill + a directional sun.
    static constexpr float kAmbR = 0.25f;
    static constexpr float kAmbG = 0.25f;
    static constexpr float kAmbB = 0.28f;
    static constexpr float kDirX = -0.4f;
    static constexpr float kDirZ = -0.6f;
    static constexpr float kDirG = 0.97f;
    static constexpr float kDirB = 0.90f;
    static constexpr float kDirIntensity = 1.5f;

    ambient_light_ = std::make_shared<vne::scene::AmbientLight>(
        vne::math::Vec3f{kAmbR, kAmbG, kAmbB}, 1.0f, "MeshAmbient");
    scene_state_.addLight(ambient_light_);

    dir_light_ = std::make_shared<vne::scene::DirectionalLight>(
        vne::math::Vec3f{kDirX, -1.0f, kDirZ},
        vne::math::Vec3f{1.0f, kDirG, kDirB},
        kDirIntensity,
        "MeshDirectional");
    scene_state_.addLight(dir_light_);
}

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
    model_ = vne::math::Mat4f::identity();
    for (int k = 0; k < 3; ++k) {
        aabb_min_[k] = 0.0f;
        aabb_max_[k] = 0.0f;
    }
    mesh_path_ = std::move(path);
    loadMeshFromPath();
}

void MeshLayer::loadMeshFromPath() {
#ifdef VNE_TESTBED_VNEIO
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
        VNE_LOG_ERROR << "MeshLayer: mesh is empty: " << mesh_path_;
#endif
        return;
    }
    const size_t vertex_count = mesh.getVertexCount();
    const size_t index_count = mesh.getIndexCount();

    // Pack interleaved VBO: position(3) + normal(3) + color(3) to match PhongMaterial layout.
    std::vector<float> vbo_data;
    static constexpr size_t kFloatsPerVertex = 9u;  // pos(3) + normal(3) + color(3)
    vbo_data.reserve(vertex_count * kFloatsPerVertex);
    constexpr float kColorR = 0.72f;
    constexpr float kColorG = 0.72f;
    constexpr float kColorB = 0.76f;
    for (size_t i = 0; i < vertex_count; ++i) {
        const auto& v = mesh.vertices[i];
        vbo_data.push_back(v.position[0]);
        vbo_data.push_back(v.position[1]);
        vbo_data.push_back(v.position[2]);
        vbo_data.push_back(v.normal[0]);
        vbo_data.push_back(v.normal[1]);
        vbo_data.push_back(v.normal[2]);
        vbo_data.push_back(kColorR);
        vbo_data.push_back(kColorG);
        vbo_data.push_back(kColorB);
    }
    vbo_ = device_->createVertexBuffer(vbo_data.data(), static_cast<uint32_t>(vbo_data.size() * sizeof(float)));
    ibo_ = device_->createIndexBuffer(mesh.indices.data(), static_cast<uint32_t>(index_count));
    index_count_ = static_cast<uint32_t>(index_count);
    ready_ = vbo_.isValid() && ibo_.isValid();

    for (int k = 0; k < 3; ++k) {
        aabb_min_[k] = mesh.aabb_min[k];
        aabb_max_[k] = mesh.aabb_max[k];
    }
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
    const PhongLightParams lights = buildLightParams();
    mesh_renderer_->drawMesh(device_, camera.get(), vbo_, ibo_, index_count_, model_, lights);
}

PhongLightParams MeshLayer::buildLightParams() const {
    PhongLightParams out{};
    // Disable defaults — everything comes from scene_state_ lights.
    out.ambient_intensity = 0.0f;
    out.dir_light_enabled = false;

    int point_idx = 0;
    for (const auto& light : scene_state_.getLights()) {
        if (!light || !light->isEnabled()) {
            continue;
        }
        switch (light->getLightType()) {
            case vne::scene::LightType::eAmbient:
                out.ambient_color = light->getColor();
                out.ambient_intensity = light->getIntensity();
                break;

            case vne::scene::LightType::eDirectional:
                // Use the first enabled directional light found.
                if (!out.dir_light_enabled) {
                    out.dir_light_enabled = true;
                    out.dir_light_dir = light->getDirection();
                    out.dir_light_color = light->getColor();
                    out.dir_light_intensity = light->getIntensity();
                }
                break;

            case vne::scene::LightType::ePoint:
                if (point_idx < PhongLightParams::kMaxPointLights) {
                    auto& pt = out.point_lights[point_idx++];
                    pt.enabled = true;
                    pt.position = light->getPosition();
                    pt.color = light->getColor();
                    pt.intensity = light->getIntensity();
                    if (const auto* pl = dynamic_cast<const vne::scene::PointLight*>(light.get())) {
                        pt.range = pl->getRange();
                    }
                }
                break;

            case vne::scene::LightType::eSpot:
                // First enabled spot light only.
                if (!out.spot_light.enabled) {
                    out.spot_light.enabled = true;
                    out.spot_light.position = light->getPosition();
                    out.spot_light.direction = light->getDirection();
                    out.spot_light.color = light->getColor();
                    out.spot_light.intensity = light->getIntensity();
                }
                break;
        }
    }
    return out;
}

}  // namespace testbed
}  // namespace vne
