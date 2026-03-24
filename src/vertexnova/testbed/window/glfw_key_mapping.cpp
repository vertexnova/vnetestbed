/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/window/glfw_key_mapping.h"

#include "vertexnova/events/types.h"

#include <GLFW/glfw3.h>

namespace vne {
namespace testbed {
namespace window {

namespace {

constexpr int kGlfwKeyMin = -1;
constexpr int kGlfwKeyMax = 348;  // GLFW_KEY_MENU

}  // namespace

vne::events::KeyCode mapGlfwToKeyCode(int glfw_key) {
    if (glfw_key < kGlfwKeyMin || glfw_key > kGlfwKeyMax) {
        return vne::events::KeyCode::eUnknown;
    }
    return static_cast<vne::events::KeyCode>(glfw_key);
}

vne::events::MouseButton mapGlfwToMouseButton(int glfw_button) {
    if (glfw_button >= GLFW_MOUSE_BUTTON_1 && glfw_button <= GLFW_MOUSE_BUTTON_8) {
        return static_cast<vne::events::MouseButton>(glfw_button);
    }
    return vne::events::MouseButton::eLeft;
}

uint8_t mapGlfwToModifiers(int glfw_mods) {
    using namespace vne::events;
    uint8_t out = 0;
    if (glfw_mods & GLFW_MOD_SHIFT) {
        out |= static_cast<uint8_t>(ModifierKey::eModShift);
    }
    if (glfw_mods & GLFW_MOD_CONTROL) {
        out |= static_cast<uint8_t>(ModifierKey::eModCtrl);
    }
    if (glfw_mods & GLFW_MOD_ALT) {
        out |= static_cast<uint8_t>(ModifierKey::eModAlt);
    }
    if (glfw_mods & GLFW_MOD_SUPER) {
        out |= static_cast<uint8_t>(ModifierKey::eModSuper);
    }
    return out;
}

}  // namespace window
}  // namespace testbed
}  // namespace vne
