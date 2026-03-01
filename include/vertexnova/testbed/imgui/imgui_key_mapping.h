#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Mapping from vne::events key/mouse/modifier types to ImGui equivalents.
 * Used by ImGuiEventListener for manual event forwarding.
 * ----------------------------------------------------------------------
 */

#if !defined(VNE_TESTBED_IMGUI)
// Stub: ImGui key mapping functions are unavailable when VNE_TESTBED_IMGUI is not defined.
// Build with -DVNE_TESTBED_IMGUI to enable. Safe for IDE indexers and doc generators.
#else

#include "vertexnova/events/types.h"

#include <imgui.h>

namespace vne {
namespace testbed {

/** @brief Map vne::events::KeyCode to ImGuiKey. Returns ImGuiKey_None if not mapped. */
ImGuiKey mapKeyCodeToImGui(vne::events::KeyCode key_code);

/** @brief Map vne::events::MouseButton to ImGui button index (0=left, 1=right, 2=middle). Returns -1 if not mapped. */
int mapMouseButtonToImGui(vne::events::MouseButton button);

/** @brief Map vne::events modifier flags to ImGui modifier flags. */
void updateImGuiModifiers(uint8_t modifiers);

}  // namespace testbed
}  // namespace vne

#endif  // VNE_TESTBED_IMGUI
