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

namespace vne {
namespace testbed_ns {

/**
 * @brief Pluggable rendering backend interface for the testbed.
 *
 * Two concrete adapters are planned:
 *  - OpenGL adapter (immediate-mode debug rendering, gizmos)
 *  - CrossGL adapter (validates the CrossGL rendering backend)
 *
 * The testbed selects an adapter at startup (e.g. via --renderer flag)
 * and calls the hooks each frame.  Plugins receive a pointer to the
 * active adapter when they need to issue draw calls.
 */
class IRenderAdapter {
public:
    virtual ~IRenderAdapter() = default;

    /**
     * @brief Initialise the adapter and create the rendering context.
     * @param window_handle Platform-specific window handle (e.g. GLFWwindow*).
     * @return true on success, false if initialisation failed.
     */
    virtual bool init(void* window_handle) = 0;

    /** @brief Begin a new frame (clear buffers, start ImGui frame, etc.). */
    virtual void beginFrame() = 0;

    /** @brief End the current frame and present / swap buffers. */
    virtual void endFrame() = 0;

    /** @brief Release all GPU resources and shut down the context. */
    virtual void shutdown() = 0;
};

}  // namespace testbed_ns
}  // namespace vne
