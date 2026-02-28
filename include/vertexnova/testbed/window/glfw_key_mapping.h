#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * GLFW → vne::events key/mouse/modifier mapping (xwin-style).
 * ----------------------------------------------------------------------
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "glfw_key_mapping.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES."
#endif

#include "vertexnova/events/types.h"

#include <GLFW/glfw3.h>

#include <cstdint>

namespace vne {
namespace testbed {
namespace window {

/** @brief Map GLFW key code to vne::events::KeyCode. */
vne::events::KeyCode mapGlfwToKeyCode(int glfw_key);

/** @brief Map GLFW mouse button to vne::events::MouseButton. */
vne::events::MouseButton mapGlfwToMouseButton(int glfw_button);

/** @brief Map GLFW modifier flags to vne::events modifier bitfield (ModifierKey). */
uint8_t mapGlfwToModifiers(int glfw_mods);

}  // namespace window
}  // namespace testbed
}  // namespace vne
