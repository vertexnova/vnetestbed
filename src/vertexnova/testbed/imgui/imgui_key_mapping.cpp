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

#include "vertexnova/testbed/imgui/imgui_key_mapping.h"

#include "vertexnova/events/key_event.h"

namespace vne {
namespace testbed {

ImGuiKey mapKeyCodeToImGui(vne::events::KeyCode key_code) {
    using K = vne::events::KeyCode;
    switch (key_code) {
        case K::eUnknown:
            return ImGuiKey_None;
        case K::eSpace:
            return ImGuiKey_Space;
        case K::eApostrophe:
            return ImGuiKey_Apostrophe;
        case K::eComma:
            return ImGuiKey_Comma;
        case K::eMinus:
            return ImGuiKey_Minus;
        case K::ePeriod:
            return ImGuiKey_Period;
        case K::eSlash:
            return ImGuiKey_Slash;
        case K::e0:
            return ImGuiKey_0;
        case K::e1:
            return ImGuiKey_1;
        case K::e2:
            return ImGuiKey_2;
        case K::e3:
            return ImGuiKey_3;
        case K::e4:
            return ImGuiKey_4;
        case K::e5:
            return ImGuiKey_5;
        case K::e6:
            return ImGuiKey_6;
        case K::e7:
            return ImGuiKey_7;
        case K::e8:
            return ImGuiKey_8;
        case K::e9:
            return ImGuiKey_9;
        case K::eSemicolon:
            return ImGuiKey_Semicolon;
        case K::eEqual:
            return ImGuiKey_Equal;
        case K::eA:
            return ImGuiKey_A;
        case K::eB:
            return ImGuiKey_B;
        case K::eC:
            return ImGuiKey_C;
        case K::eD:
            return ImGuiKey_D;
        case K::eE:
            return ImGuiKey_E;
        case K::eF:
            return ImGuiKey_F;
        case K::eG:
            return ImGuiKey_G;
        case K::eH:
            return ImGuiKey_H;
        case K::eI:
            return ImGuiKey_I;
        case K::eJ:
            return ImGuiKey_J;
        case K::eK:
            return ImGuiKey_K;
        case K::eL:
            return ImGuiKey_L;
        case K::eM:
            return ImGuiKey_M;
        case K::eN:
            return ImGuiKey_N;
        case K::eO:
            return ImGuiKey_O;
        case K::eP:
            return ImGuiKey_P;
        case K::eQ:
            return ImGuiKey_Q;
        case K::eR:
            return ImGuiKey_R;
        case K::eS:
            return ImGuiKey_S;
        case K::eT:
            return ImGuiKey_T;
        case K::eU:
            return ImGuiKey_U;
        case K::eV:
            return ImGuiKey_V;
        case K::eW:
            return ImGuiKey_W;
        case K::eX:
            return ImGuiKey_X;
        case K::eY:
            return ImGuiKey_Y;
        case K::eZ:
            return ImGuiKey_Z;
        case K::eLeftBracket:
            return ImGuiKey_LeftBracket;
        case K::eBackslash:
            return ImGuiKey_Backslash;
        case K::eRightBracket:
            return ImGuiKey_RightBracket;
        case K::eGraveAccent:
            return ImGuiKey_GraveAccent;
        case K::eEscape:
            return ImGuiKey_Escape;
        case K::eEnter:
            return ImGuiKey_Enter;
        case K::eTab:
            return ImGuiKey_Tab;
        case K::eBackspace:
            return ImGuiKey_Backspace;
        case K::eInsert:
            return ImGuiKey_Insert;
        case K::eDelete:
            return ImGuiKey_Delete;
        case K::eRight:
            return ImGuiKey_RightArrow;
        case K::eLeft:
            return ImGuiKey_LeftArrow;
        case K::eDown:
            return ImGuiKey_DownArrow;
        case K::eUp:
            return ImGuiKey_UpArrow;
        case K::ePageUp:
            return ImGuiKey_PageUp;
        case K::ePageDown:
            return ImGuiKey_PageDown;
        case K::eHome:
            return ImGuiKey_Home;
        case K::eEnd:
            return ImGuiKey_End;
        case K::eCapsLock:
            return ImGuiKey_CapsLock;
        case K::eScrollLock:
            return ImGuiKey_ScrollLock;
        case K::eNumLock:
            return ImGuiKey_NumLock;
        case K::ePrintScreen:
            return ImGuiKey_PrintScreen;
        case K::ePause:
            return ImGuiKey_Pause;
        case K::eF1:
            return ImGuiKey_F1;
        case K::eF2:
            return ImGuiKey_F2;
        case K::eF3:
            return ImGuiKey_F3;
        case K::eF4:
            return ImGuiKey_F4;
        case K::eF5:
            return ImGuiKey_F5;
        case K::eF6:
            return ImGuiKey_F6;
        case K::eF7:
            return ImGuiKey_F7;
        case K::eF8:
            return ImGuiKey_F8;
        case K::eF9:
            return ImGuiKey_F9;
        case K::eF10:
            return ImGuiKey_F10;
        case K::eF11:
            return ImGuiKey_F11;
        case K::eF12:
            return ImGuiKey_F12;
        case K::eF13:
            return ImGuiKey_F13;
        case K::eF14:
            return ImGuiKey_F14;
        case K::eF15:
            return ImGuiKey_F15;
        case K::eF16:
            return ImGuiKey_F16;
        case K::eF17:
            return ImGuiKey_F17;
        case K::eF18:
            return ImGuiKey_F18;
        case K::eF19:
            return ImGuiKey_F19;
        case K::eF20:
            return ImGuiKey_F20;
        case K::eF21:
            return ImGuiKey_F21;
        case K::eF22:
            return ImGuiKey_F22;
        case K::eF23:
            return ImGuiKey_F23;
        case K::eF24:
            return ImGuiKey_F24;
        case K::eKp0:
            return ImGuiKey_Keypad0;
        case K::eKp1:
            return ImGuiKey_Keypad1;
        case K::eKp2:
            return ImGuiKey_Keypad2;
        case K::eKp3:
            return ImGuiKey_Keypad3;
        case K::eKp4:
            return ImGuiKey_Keypad4;
        case K::eKp5:
            return ImGuiKey_Keypad5;
        case K::eKp6:
            return ImGuiKey_Keypad6;
        case K::eKp7:
            return ImGuiKey_Keypad7;
        case K::eKp8:
            return ImGuiKey_Keypad8;
        case K::eKp9:
            return ImGuiKey_Keypad9;
        case K::eKpDecimal:
            return ImGuiKey_KeypadDecimal;
        case K::eKpDivide:
            return ImGuiKey_KeypadDivide;
        case K::eKpMultiply:
            return ImGuiKey_KeypadMultiply;
        case K::eKpSubtract:
            return ImGuiKey_KeypadSubtract;
        case K::eKpAdd:
            return ImGuiKey_KeypadAdd;
        case K::eKpEnter:
            return ImGuiKey_KeypadEnter;
        case K::eKpEqual:
            return ImGuiKey_KeypadEqual;
        case K::eLeftShift:
            return ImGuiKey_LeftShift;
        case K::eLeftControl:
            return ImGuiKey_LeftCtrl;
        case K::eLeftAlt:
            return ImGuiKey_LeftAlt;
        case K::eLeftSuper:
            return ImGuiKey_LeftSuper;
        case K::eRightShift:
            return ImGuiKey_RightShift;
        case K::eRightControl:
            return ImGuiKey_RightCtrl;
        case K::eRightAlt:
            return ImGuiKey_RightAlt;
        case K::eRightSuper:
            return ImGuiKey_RightSuper;
        case K::eMenu:
            return ImGuiKey_Menu;
        default:
            return ImGuiKey_None;
    }
}

int mapMouseButtonToImGui(vne::events::MouseButton button) {
    using M = vne::events::MouseButton;
    switch (button) {
        case M::eLeft:
            return ImGuiMouseButton_Left;
        case M::eRight:
            return ImGuiMouseButton_Right;
        case M::eMiddle:
            return ImGuiMouseButton_Middle;
        default:
            return -1;
    }
}

void updateImGuiModifiers(uint8_t modifiers) {
    using MK = vne::events::ModifierKey;
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (modifiers & static_cast<uint8_t>(MK::eModCtrl)) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (modifiers & static_cast<uint8_t>(MK::eModShift)) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (modifiers & static_cast<uint8_t>(MK::eModAlt)) != 0);
    io.AddKeyEvent(ImGuiMod_Super,
                   (modifiers
                    & (static_cast<uint8_t>(MK::eModSuper) | static_cast<uint8_t>(MK::eModMeta)
                       | static_cast<uint8_t>(MK::eModCmd)))
                       != 0);
}

}  // namespace testbed
}  // namespace vne
