#pragma once

#include "imgui/imgui.h"
#include "VulkanSurfer/VulkanSurfer.h"
#include <chrono>
#include <vector>

// Backend State
struct ImGui_ImplVulkanSurfer_Data {
    Surfer::Window *Window;
    std::chrono::time_point<std::chrono::high_resolution_clock> Time;

    // Event queues
    struct KeyEvent { ImGuiKey key; bool down; };
    struct MouseButtonEvent { int button; bool down; };
    struct MousePosEvent { float x, y; };

    std::vector<KeyEvent> KeyEvents;
    std::vector<MouseButtonEvent> MouseButtonEvents;
    std::vector<MousePosEvent> MousePosEvents;
    ImVec2 DisplaySize;
};

// Use a static pointer to keep the state local to the translation unit
static ImGui_ImplVulkanSurfer_Data *g_VulkanSurferData = nullptr;

// Helper: Key Mapping
inline ImGuiKey ImGui_ImplVulkanSurfer_KeyToImGuiKey(Surfer::KeyCode key) {
    switch (key) {
        // Mouse buttons are handled separately, not as ImGuiKey
        case Surfer::KeyCode::MouseLeft:
        case Surfer::KeyCode::MouseRight:
        case Surfer::KeyCode::MouseMiddle:
            return ImGuiKey_None;

        case Surfer::KeyCode::Tab: return ImGuiKey_Tab;
        case Surfer::KeyCode::ArrowLeft: return ImGuiKey_LeftArrow;
        case Surfer::KeyCode::ArrowRight: return ImGuiKey_RightArrow;
        case Surfer::KeyCode::ArrowUp: return ImGuiKey_UpArrow;
        case Surfer::KeyCode::ArrowDown: return ImGuiKey_DownArrow;
        case Surfer::KeyCode::PageUp: return ImGuiKey_PageUp;
        case Surfer::KeyCode::PageDown: return ImGuiKey_PageDown;
        case Surfer::KeyCode::Home: return ImGuiKey_Home;
        case Surfer::KeyCode::End: return ImGuiKey_End;
        case Surfer::KeyCode::Insert: return ImGuiKey_Insert;
        case Surfer::KeyCode::BackSpace: return ImGuiKey_Backspace;
        case Surfer::KeyCode::Space: return ImGuiKey_Space;
        case Surfer::KeyCode::Enter: return ImGuiKey_Enter;
        case Surfer::KeyCode::Esc: return ImGuiKey_Escape;

        // Alphabet
        case Surfer::KeyCode::KeyA: return ImGuiKey_A;
        case Surfer::KeyCode::KeyB: return ImGuiKey_B;
        case Surfer::KeyCode::KeyC: return ImGuiKey_C;
        case Surfer::KeyCode::KeyD: return ImGuiKey_D;
        case Surfer::KeyCode::KeyE: return ImGuiKey_E;
        case Surfer::KeyCode::KeyF: return ImGuiKey_F;
        case Surfer::KeyCode::KeyG: return ImGuiKey_G;
        case Surfer::KeyCode::KeyH: return ImGuiKey_H;
        case Surfer::KeyCode::KeyI: return ImGuiKey_I;
        case Surfer::KeyCode::KeyJ: return ImGuiKey_J;
        case Surfer::KeyCode::KeyK: return ImGuiKey_K;
        case Surfer::KeyCode::KeyL: return ImGuiKey_L;
        case Surfer::KeyCode::KeyM: return ImGuiKey_M;
        case Surfer::KeyCode::KeyN: return ImGuiKey_N;
        case Surfer::KeyCode::KeyO: return ImGuiKey_O;
        case Surfer::KeyCode::KeyP: return ImGuiKey_P;
        case Surfer::KeyCode::KeyQ: return ImGuiKey_Q;
        case Surfer::KeyCode::KeyR: return ImGuiKey_R;
        case Surfer::KeyCode::KeyS: return ImGuiKey_S;
        case Surfer::KeyCode::KeyT: return ImGuiKey_T;
        case Surfer::KeyCode::KeyU: return ImGuiKey_U;
        case Surfer::KeyCode::KeyV: return ImGuiKey_V;
        case Surfer::KeyCode::KeyW: return ImGuiKey_W;
        case Surfer::KeyCode::KeyX: return ImGuiKey_X;
        case Surfer::KeyCode::KeyY: return ImGuiKey_Y;
        case Surfer::KeyCode::KeyZ: return ImGuiKey_Z;

        // Numbers
        case Surfer::KeyCode::Num0: return ImGuiKey_0;
        case Surfer::KeyCode::Num1: return ImGuiKey_1;
        case Surfer::KeyCode::Num2: return ImGuiKey_2;
        case Surfer::KeyCode::Num3: return ImGuiKey_3;
        case Surfer::KeyCode::Num4: return ImGuiKey_4;
        case Surfer::KeyCode::Num5: return ImGuiKey_5;
        case Surfer::KeyCode::Num6: return ImGuiKey_6;
        case Surfer::KeyCode::Num7: return ImGuiKey_7;
        case Surfer::KeyCode::Num8: return ImGuiKey_8;
        case Surfer::KeyCode::Num9: return ImGuiKey_9;

        // Numpad
        case Surfer::KeyCode::Numpad0: return ImGuiKey_Keypad0;
        case Surfer::KeyCode::Numpad1: return ImGuiKey_Keypad1;
        case Surfer::KeyCode::Numpad2: return ImGuiKey_Keypad2;
        case Surfer::KeyCode::Numpad3: return ImGuiKey_Keypad3;
        case Surfer::KeyCode::Numpad4: return ImGuiKey_Keypad4;
        case Surfer::KeyCode::Numpad5: return ImGuiKey_Keypad5;
        case Surfer::KeyCode::Numpad6: return ImGuiKey_Keypad6;
        case Surfer::KeyCode::Numpad7: return ImGuiKey_Keypad7;
        case Surfer::KeyCode::Numpad8: return ImGuiKey_Keypad8;
        case Surfer::KeyCode::Numpad9: return ImGuiKey_Keypad9;
        case Surfer::KeyCode::NumpadEnter: return ImGuiKey_KeypadEnter;
        case Surfer::KeyCode::NumpadMultiply: return ImGuiKey_KeypadMultiply;
        case Surfer::KeyCode::NumpadSubtract: return ImGuiKey_KeypadSubtract;
        case Surfer::KeyCode::NumpadAdd: return ImGuiKey_KeypadAdd;
        case Surfer::KeyCode::NumpadDecimal: return ImGuiKey_KeypadDecimal;
        case Surfer::KeyCode::NumpadDivide: return ImGuiKey_KeypadDivide;
        case Surfer::KeyCode::NumpadEqual: return ImGuiKey_KeypadEqual;

        // Function Keys
        case Surfer::KeyCode::F1: return ImGuiKey_F1;
        case Surfer::KeyCode::F2: return ImGuiKey_F2;
        case Surfer::KeyCode::F3: return ImGuiKey_F3;
        case Surfer::KeyCode::F4: return ImGuiKey_F4;
        case Surfer::KeyCode::F5: return ImGuiKey_F5;
        case Surfer::KeyCode::F6: return ImGuiKey_F6;
        case Surfer::KeyCode::F7: return ImGuiKey_F7;
        case Surfer::KeyCode::F8: return ImGuiKey_F8;
        case Surfer::KeyCode::F9: return ImGuiKey_F9;
        case Surfer::KeyCode::F10: return ImGuiKey_F10;
        case Surfer::KeyCode::F11: return ImGuiKey_F11;
        case Surfer::KeyCode::F12: return ImGuiKey_F12;

        // Modifiers
        case Surfer::KeyCode::LeftShift: return ImGuiKey_LeftShift;
        case Surfer::KeyCode::RightShift: return ImGuiKey_RightShift;
        case Surfer::KeyCode::LeftControl: return ImGuiKey_LeftCtrl;
        case Surfer::KeyCode::RightControl: return ImGuiKey_RightCtrl;
        case Surfer::KeyCode::LeftAlt: return ImGuiKey_LeftAlt;
        case Surfer::KeyCode::RightAlt: return ImGuiKey_RightAlt;
        case Surfer::KeyCode::SuperLeft: return ImGuiKey_LeftSuper;
        case Surfer::KeyCode::SuperRight: return ImGuiKey_RightSuper;
        case Surfer::KeyCode::CapsLock: return ImGuiKey_CapsLock;
        case Surfer::KeyCode::NumLock: return ImGuiKey_NumLock;
        case Surfer::KeyCode::ScrollLock: return ImGuiKey_ScrollLock;

        // Editing
        case Surfer::KeyCode::Delete: return ImGuiKey_Delete;

        // Special
        case Surfer::KeyCode::PrtSc: return ImGuiKey_PrintScreen;
        case Surfer::KeyCode::Pause: return ImGuiKey_Pause;

        // Symbols
        case Surfer::KeyCode::Minus: return ImGuiKey_Minus;
        case Surfer::KeyCode::Equal: return ImGuiKey_Equal;
        case Surfer::KeyCode::BracketLeft: return ImGuiKey_LeftBracket;
        case Surfer::KeyCode::BracketRight: return ImGuiKey_RightBracket;
        case Surfer::KeyCode::Semicolon: return ImGuiKey_Semicolon;
        case Surfer::KeyCode::Apostrophe: return ImGuiKey_Apostrophe;
        case Surfer::KeyCode::Comma: return ImGuiKey_Comma;
        case Surfer::KeyCode::Period: return ImGuiKey_Period;
        case Surfer::KeyCode::Slash: return ImGuiKey_Slash;
        case Surfer::KeyCode::Backslash: return ImGuiKey_Backslash;
        case Surfer::KeyCode::Grave: return ImGuiKey_GraveAccent;

        // Mouse Wheel (ImGui handles these separately, not as keys)
        case Surfer::KeyCode::MouseWheelUp: return ImGuiKey_None;
        case Surfer::KeyCode::MouseWheelDown: return ImGuiKey_None;

        default: return ImGuiKey_None;
    }
}

// Helper: Convert Surfer mouse button to ImGui mouse button index
inline int ImGui_ImplVulkanSurfer_KeyToMouseButton(Surfer::KeyCode key) {
    switch (key) {
        case Surfer::KeyCode::MouseLeft: return 0;
        case Surfer::KeyCode::MouseRight: return 1;
        case Surfer::KeyCode::MouseMiddle: return 2;
        default: return -1;
    }
}

// API Implementation

inline bool ImGui_ImplVulkanSurfer_Init(Surfer::Window *window) {
    ImGuiIO &io = ImGui::GetIO();
    std::uint32_t width, height;
    window->getWindowSize(width, height);

    // Check if backend is already initialized
    IM_ASSERT(g_VulkanSurferData == nullptr && "Already initialized!");

    // Initialize state
    g_VulkanSurferData = new ImGui_ImplVulkanSurfer_Data();
    g_VulkanSurferData->Window = window;
    g_VulkanSurferData->Time = std::chrono::high_resolution_clock::now();
    g_VulkanSurferData->DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

    io.BackendPlatformName = "imgui_impl_vulkansurfer_header_only";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    // Keyboard Callbacks - Queue events instead of directly calling ImGui
    window->registerKeyPressCallback([](Surfer::KeyCode key) {
        if (g_VulkanSurferData) {
            // Check if it's a mouse button
            int mouse_button = ImGui_ImplVulkanSurfer_KeyToMouseButton(key);
            if (mouse_button >= 0) {
                g_VulkanSurferData->MouseButtonEvents.push_back({mouse_button, true});
            } else {
                ImGuiKey imgui_key = ImGui_ImplVulkanSurfer_KeyToImGuiKey(key);
                if (imgui_key != ImGuiKey_None) {
                    g_VulkanSurferData->KeyEvents.push_back({imgui_key, true});
                }
            }
        }
    });

    window->registerKeyReleaseCallback([](Surfer::KeyCode key) {
        if (g_VulkanSurferData) {
            // Check if it's a mouse button
            int mouse_button = ImGui_ImplVulkanSurfer_KeyToMouseButton(key);
            if (mouse_button >= 0) {
                g_VulkanSurferData->MouseButtonEvents.push_back({mouse_button, false});
            } else {
                ImGuiKey imgui_key = ImGui_ImplVulkanSurfer_KeyToImGuiKey(key);
                if (imgui_key != ImGuiKey_None) {
                    g_VulkanSurferData->KeyEvents.push_back({imgui_key, false});
                }
            }
        }
    });

    // Mouse Input (Motion & Buttons) - Queue events
    window->registerMouseMotionCallback([](int x, int y) {
        if (g_VulkanSurferData) {
            g_VulkanSurferData->MousePosEvents.push_back({(float)x, (float)y});
        }
    });

    // Resize Handling
    window->registerResizeCallback([](int width, int height) {
        if (g_VulkanSurferData) {
            g_VulkanSurferData->DisplaySize = ImVec2((float)width, (float)height);
        }
    });

    return true;
}

inline void ImGui_ImplVulkanSurfer_Shutdown() {
    if (g_VulkanSurferData) {
        delete g_VulkanSurferData;
        g_VulkanSurferData = nullptr;
    }
}

inline void ImGui_ImplVulkanSurfer_NewFrame() {
    IM_ASSERT(g_VulkanSurferData != nullptr && "Backend not initialized!");
    ImGuiIO &io = ImGui::GetIO();

    // Update display size
    io.DisplaySize = g_VulkanSurferData->DisplaySize;

    // Setup Delta Time
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> delta_time = current_time - g_VulkanSurferData->Time;
    io.DeltaTime = delta_time.count() > 0.0f ? delta_time.count() : (1.0f / 60.0f);
    g_VulkanSurferData->Time = current_time;

    // Process queued keyboard events
    for (const auto& event : g_VulkanSurferData->KeyEvents) {
        io.AddKeyEvent(event.key, event.down);
    }
    g_VulkanSurferData->KeyEvents.clear();

    // Process queued mouse button events
    for (const auto& event : g_VulkanSurferData->MouseButtonEvents) {
        io.AddMouseButtonEvent(event.button, event.down);
    }
    g_VulkanSurferData->MouseButtonEvents.clear();

    // Process queued mouse position events
    for (const auto& event : g_VulkanSurferData->MousePosEvents) {
        io.AddMousePosEvent(event.x, event.y);
    }
    g_VulkanSurferData->MousePosEvents.clear();
}