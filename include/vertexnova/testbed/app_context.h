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
 * @file app_context.h
 * @brief Backend-agnostic app context: window, renderer adapter, optional debugDraw.
 * Core does not depend on GLFW/OpenGL; the runner supplies concrete implementations.
 */

namespace vne {
namespace testbed {

/** @brief Minimal window interface (size, poll); implemented by runner with GLFW etc. */
struct IWindow {
    virtual ~IWindow() = default;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() const = 0;
};

/** @brief Minimal renderer adapter (begin/end frame); implemented by runner. */
struct IRendererAdapter {
    virtual ~IRendererAdapter() = default;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
};

/** @brief Optional debug draw; can be nullptr. */
struct IDebugDraw {
    virtual ~IDebugDraw() = default;
    virtual void draw() = 0;
};

/**
 * @struct AppContext
 * @brief Opaque container filled by the runner; plugins access window/renderer/debugDraw.
 */
struct AppContext {
    IWindow* window = nullptr;
    IRendererAdapter* renderer = nullptr;
    IDebugDraw* debugDraw = nullptr;
};

}  // namespace testbed
}  // namespace vne
