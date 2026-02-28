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
 * @file gl/buffer_layout.h
 * @brief Vertex buffer layout: ShaderDataType, BufferElement, BufferLayout.
 *
 * Used with VertexArray::addVertexBuffer(vb, layout) to describe how vertex
 * buffer bytes map to shader attribute locations (stride, offsets, types).
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "buffer_layout.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES. Build with OpenGL or OpenGL ES enabled."
#endif

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @enum ShaderDataType
 * @brief GLSL-compatible data types for vertex attributes.
 */
enum class ShaderDataType {
    eNone = 0,
    eFloat = 1,
    eFloat2 = 2,
    eFloat3 = 3,
    eFloat4 = 4,
    eMat3 = 5,
    eMat4 = 6,
    eInt = 7,
    eInt2 = 8,
    eInt3 = 9,
    eInt4 = 10,
    eBool = 11
};

/**
 * @brief Size in bytes for a ShaderDataType.
 */
inline std::size_t shaderDataTypeSize(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::eFloat:
            return 4;
        case ShaderDataType::eFloat2:
            return 4 * 2;
        case ShaderDataType::eFloat3:
            return 4 * 3;
        case ShaderDataType::eFloat4:
            return 4 * 4;
        case ShaderDataType::eMat3:
            return 4 * 3 * 3;
        case ShaderDataType::eMat4:
            return 4 * 4 * 4;
        case ShaderDataType::eInt:
            return 4;
        case ShaderDataType::eInt2:
            return 4 * 2;
        case ShaderDataType::eInt3:
            return 4 * 3;
        case ShaderDataType::eInt4:
            return 4 * 4;
        case ShaderDataType::eBool:
            return 1;
        case ShaderDataType::eNone:
        default:
            return 0;
    }
}

/**
 * @struct BufferElement
 * @brief One attribute within a vertex buffer layout (name, type, offset, normalized).
 */
struct BufferElement {
    std::string name;
    ShaderDataType type{ShaderDataType::eNone};
    std::size_t size{0};
    std::size_t offset{0};
    bool normalized{false};

    BufferElement() = default;

    BufferElement(ShaderDataType type_in, const std::string& name_in, bool normalized_in = false)
        : name(name_in)
        , type(type_in)
        , size(shaderDataTypeSize(type_in))
        , offset(0)
        , normalized(normalized_in) {}

    /**
     * @brief Number of components (e.g. 3 for Float3, 4 for Mat4 as 4*float4).
     */
    uint32_t getComponentCount() const {
        switch (type) {
            case ShaderDataType::eFloat:
                return 1;
            case ShaderDataType::eFloat2:
                return 2;
            case ShaderDataType::eFloat3:
                return 3;
            case ShaderDataType::eFloat4:
                return 4;
            case ShaderDataType::eMat3:
                return 3;  // 3 * float3
            case ShaderDataType::eMat4:
                return 4;  // 4 * float4
            case ShaderDataType::eInt:
                return 1;
            case ShaderDataType::eInt2:
                return 2;
            case ShaderDataType::eInt3:
                return 3;
            case ShaderDataType::eInt4:
                return 4;
            case ShaderDataType::eBool:
                return 1;
            case ShaderDataType::eNone:
            default:
                return 0;
        }
    }
};

/**
 * @class BufferLayout
 * @brief Layout description for a vertex buffer (elements, stride, offsets).
 */
class BufferLayout {
   public:
    BufferLayout() = default;

    BufferLayout(std::initializer_list<BufferElement> elements)
        : elements_(elements) {
        calculateOffsetsAndStride();
    }

    [[nodiscard]] std::size_t getStride() const { return stride_; }
    [[nodiscard]] const std::vector<BufferElement>& getElements() const { return elements_; }

    std::vector<BufferElement>::iterator begin() { return elements_.begin(); }
    std::vector<BufferElement>::iterator end() { return elements_.end(); }
    std::vector<BufferElement>::const_iterator begin() const { return elements_.begin(); }
    std::vector<BufferElement>::const_iterator end() const { return elements_.end(); }

   private:
    void calculateOffsetsAndStride() {
        std::size_t offset = 0;
        stride_ = 0;
        for (auto& element : elements_) {
            element.offset = offset;
            offset += element.size;
            stride_ += element.size;
        }
    }

    std::vector<BufferElement> elements_;
    std::size_t stride_{0};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
