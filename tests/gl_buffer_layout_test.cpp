#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)

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
 * @file gl_buffer_layout_test.cpp
 * @brief White-box unit tests for BufferLayout, BufferElement, ShaderDataType (gl layer).
 */

#include <gtest/gtest.h>

#include "vertexnova/testbed/gl/buffer_layout.h"

using namespace vne::testbed::gl;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class BufferLayoutTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// ShaderDataType — shaderDataTypeSize()
// ---------------------------------------------------------------------------

TEST_F(BufferLayoutTest, ShaderDataType_Size_None) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eNone), 0u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Float) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eFloat), 4u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Float2) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eFloat2), 8u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Float3) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eFloat3), 12u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Float4) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eFloat4), 16u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Mat3) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eMat3), 36u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Mat4) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eMat4), 64u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Int) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eInt), 4u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Int2) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eInt2), 8u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Int3) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eInt3), 12u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Int4) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eInt4), 16u);
}

TEST_F(BufferLayoutTest, ShaderDataType_Size_Bool) {
    EXPECT_EQ(shaderDataTypeSize(ShaderDataType::eBool), 1u);
}

// ---------------------------------------------------------------------------
// BufferElement — getComponentCount()
// ---------------------------------------------------------------------------

TEST_F(BufferLayoutTest, BufferElement_ComponentCount_None) {
    BufferElement e(ShaderDataType::eNone, "x");
    EXPECT_EQ(e.getComponentCount(), 0u);
}

TEST_F(BufferLayoutTest, BufferElement_ComponentCount_Float) {
    BufferElement e(ShaderDataType::eFloat, "x");
    EXPECT_EQ(e.getComponentCount(), 1u);
}

TEST_F(BufferLayoutTest, BufferElement_ComponentCount_Float3) {
    BufferElement e(ShaderDataType::eFloat3, "aPos");
    EXPECT_EQ(e.getComponentCount(), 3u);
}

TEST_F(BufferLayoutTest, BufferElement_ComponentCount_Float4) {
    BufferElement e(ShaderDataType::eFloat4, "aColor");
    EXPECT_EQ(e.getComponentCount(), 4u);
}

TEST_F(BufferLayoutTest, BufferElement_ComponentCount_Mat4) {
    BufferElement e(ShaderDataType::eMat4, "uMVP");
    EXPECT_EQ(e.getComponentCount(), 4u);
}

TEST_F(BufferLayoutTest, BufferElement_ComponentCount_Int2) {
    BufferElement e(ShaderDataType::eInt2, "uv");
    EXPECT_EQ(e.getComponentCount(), 2u);
}

TEST_F(BufferLayoutTest, BufferElement_ComponentCount_Bool) {
    BufferElement e(ShaderDataType::eBool, "flag");
    EXPECT_EQ(e.getComponentCount(), 1u);
}

// ---------------------------------------------------------------------------
// BufferElement — constructor sets size from type
// ---------------------------------------------------------------------------

TEST_F(BufferLayoutTest, BufferElement_Constructor_SetsSizeFromType) {
    BufferElement e(ShaderDataType::eFloat3, "pos");
    EXPECT_EQ(e.name, "pos");
    EXPECT_EQ(e.type, ShaderDataType::eFloat3);
    EXPECT_EQ(e.size, shaderDataTypeSize(ShaderDataType::eFloat3));
    EXPECT_EQ(e.offset, 0u);
    EXPECT_FALSE(e.normalized);
}

TEST_F(BufferLayoutTest, BufferElement_Constructor_Normalized) {
    BufferElement e(ShaderDataType::eInt4, "bones", true);
    EXPECT_TRUE(e.normalized);
}

// ---------------------------------------------------------------------------
// BufferLayout — empty, single, multiple elements; stride and offsets
// ---------------------------------------------------------------------------

TEST_F(BufferLayoutTest, BufferLayout_EmptyLayout) {
    BufferLayout layout;
    EXPECT_EQ(layout.getStride(), 0u);
    EXPECT_TRUE(layout.getElements().empty());
}

TEST_F(BufferLayoutTest, BufferLayout_SingleElement) {
    BufferLayout layout{{BufferElement(ShaderDataType::eFloat3, "aPos")}};
    EXPECT_EQ(layout.getStride(), 12u);
    ASSERT_EQ(layout.getElements().size(), 1u);
    EXPECT_EQ(layout.getElements()[0].offset, 0u);
    EXPECT_EQ(layout.getElements()[0].size, 12u);
}

TEST_F(BufferLayoutTest, BufferLayout_StrideAndOffsets_TwoElements) {
    BufferLayout layout{
        {BufferElement(ShaderDataType::eFloat3, "aPos"), BufferElement(ShaderDataType::eFloat4, "aColor")}};
    EXPECT_EQ(layout.getStride(), 12u + 16u);
    ASSERT_EQ(layout.getElements().size(), 2u);
    EXPECT_EQ(layout.getElements()[0].offset, 0u);
    EXPECT_EQ(layout.getElements()[0].size, 12u);
    EXPECT_EQ(layout.getElements()[1].offset, 12u);
    EXPECT_EQ(layout.getElements()[1].size, 16u);
}

TEST_F(BufferLayoutTest, BufferLayout_Offsets_ThreeElements) {
    BufferLayout layout{{BufferElement(ShaderDataType::eFloat2, "uv"),
                         BufferElement(ShaderDataType::eFloat3, "pos"),
                         BufferElement(ShaderDataType::eFloat, "alpha")}};
    EXPECT_EQ(layout.getStride(), 8u + 12u + 4u);
    ASSERT_EQ(layout.getElements().size(), 3u);
    EXPECT_EQ(layout.getElements()[0].offset, 0u);
    EXPECT_EQ(layout.getElements()[1].offset, 8u);
    EXPECT_EQ(layout.getElements()[2].offset, 20u);
}

TEST_F(BufferLayoutTest, BufferLayout_Iterators) {
    BufferLayout layout{{BufferElement(ShaderDataType::eFloat, "x")}};
    std::size_t count = 0;
    for (const auto& elem : layout) {
        (void)elem;
        ++count;
    }
    EXPECT_EQ(count, 1u);
}

#endif  // VNE_TESTBED_OPENGL || VNE_TESTBED_OPENGLES
