#include "imgui_internal.h"

#include <cstdlib>
#include <cstring>

namespace pulse_imgui_internal {

struct pulse_imgui_platform_data {
    pulse_imgui_plugin_state* state = nullptr;
    ecs_entity_t primary_window = 0;
    SDL_Window* window = nullptr;
    SDL_WindowID window_id = 0;
    SDL_Window* ime_window = nullptr;

    char* clipboard_text = nullptr;
    SDL_Cursor* mouse_cursors[ImGuiMouseCursor_COUNT] = {};
    SDL_Cursor* last_mouse_cursor = nullptr;
    int mouse_pending_leave_frame = 0;
    int mouse_buttons_down = 0;
};

ecs_entity_t imgui_get_window_entity(ecs_world_t* world, pulse_imgui_plugin_state* state) {
    if (!world || !state) {
        return 0;
    }
    // 平台数据初始化时会缓存 primary window 实体，避免在 ECS system 回调
    // （world 处于 readonly）里调用会写 world 的 pulse_window_get_primary。
    if (state->platform_data && state->platform_data->primary_window) {
        return state->platform_data->primary_window;
    }
    return pulse_window_get_primary(state->app);
}

namespace {

pulse_imgui_platform_data* platform_data_from_imgui_context() {
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx) {
        return nullptr;
    }
    return static_cast<pulse_imgui_platform_data*>(
        ImGui::GetIO().BackendPlatformUserData);
}

SDL_Window* platform_window_from_viewport(ImGuiViewport* viewport) {
    if (!viewport || !viewport->PlatformHandle) {
        return nullptr;
    }
    return SDL_GetWindowFromID((SDL_WindowID)(intptr_t)viewport->PlatformHandle);
}

// ---------------------------------------------------------------------------
// ImGuiPlatformIO callbacks（clipboard / IME / open-url）
// ---------------------------------------------------------------------------

const char* imgui_platform_get_clipboard_text(ImGuiContext*) {
    pulse_imgui_platform_data* pd = platform_data_from_imgui_context();
    if (!pd) {
        return nullptr;
    }
    if (pd->clipboard_text) {
        SDL_free(pd->clipboard_text);
        pd->clipboard_text = nullptr;
    }
    pd->clipboard_text = SDL_GetClipboardText();
    return pd->clipboard_text;
}

void imgui_platform_set_clipboard_text(ImGuiContext*, const char* text) {
    SDL_SetClipboardText(text);
}

void imgui_platform_set_ime_data(
    ImGuiContext*,
    ImGuiViewport* viewport,
    ImGuiPlatformImeData* data
) {
    pulse_imgui_platform_data* pd = platform_data_from_imgui_context();
    if (!pd) {
        return;
    }

    SDL_Window* window = platform_window_from_viewport(viewport);
    if (!window) {
        window = pd->window;
    }

    const bool want_text = data->WantVisible || data->WantTextInput;
    if ((!want_text || pd->ime_window != window) && pd->ime_window) {
        SDL_StopTextInput(pd->ime_window);
        pd->ime_window = nullptr;
    }

    if (data->WantVisible && window) {
        SDL_Rect rect;
        rect.x = (int)(data->InputPos.x - viewport->Pos.x);
        rect.y = (int)(data->InputPos.y - viewport->Pos.y + data->InputLineHeight);
        rect.w = 1;
        rect.h = (int)data->InputLineHeight;
        SDL_SetTextInputArea(window, &rect, 0);
        pd->ime_window = window;
    }

    if (window && !SDL_TextInputActive(window) && want_text) {
        SDL_StartTextInput(window);
    }
}

bool imgui_platform_open_in_shell(ImGuiContext*, const char* url) {
    return SDL_OpenURL(url) == 0;
}

// ---------------------------------------------------------------------------
// Mouse cursor
// ---------------------------------------------------------------------------

void imgui_platform_update_mouse_cursor() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
        return;
    }

    pulse_imgui_platform_data* pd = platform_data_from_imgui_context();
    if (!pd) {
        return;
    }

    ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None) {
        SDL_HideCursor();
    } else {
        SDL_Cursor* expected = pd->mouse_cursors[cursor]
            ? pd->mouse_cursors[cursor]
            : pd->mouse_cursors[ImGuiMouseCursor_Arrow];
        if (pd->last_mouse_cursor != expected) {
            SDL_SetCursor(expected);
            pd->last_mouse_cursor = expected;
        }
        SDL_ShowCursor();
    }
}

// ---------------------------------------------------------------------------
// Key mapping（与官方 imgui_impl_sdl3 一致）
// ---------------------------------------------------------------------------

ImGuiKey imgui_sdl3_key_event_to_imgui_key(SDL_Keycode keycode, SDL_Scancode scancode) {
    // Keypad doesn't have individual key values in SDL3
    switch (scancode) {
        case SDL_SCANCODE_KP_0: return ImGuiKey_Keypad0;
        case SDL_SCANCODE_KP_1: return ImGuiKey_Keypad1;
        case SDL_SCANCODE_KP_2: return ImGuiKey_Keypad2;
        case SDL_SCANCODE_KP_3: return ImGuiKey_Keypad3;
        case SDL_SCANCODE_KP_4: return ImGuiKey_Keypad4;
        case SDL_SCANCODE_KP_5: return ImGuiKey_Keypad5;
        case SDL_SCANCODE_KP_6: return ImGuiKey_Keypad6;
        case SDL_SCANCODE_KP_7: return ImGuiKey_Keypad7;
        case SDL_SCANCODE_KP_8: return ImGuiKey_Keypad8;
        case SDL_SCANCODE_KP_9: return ImGuiKey_Keypad9;
        case SDL_SCANCODE_KP_PERIOD: return ImGuiKey_KeypadDecimal;
        case SDL_SCANCODE_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case SDL_SCANCODE_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case SDL_SCANCODE_KP_MINUS: return ImGuiKey_KeypadSubtract;
        case SDL_SCANCODE_KP_PLUS: return ImGuiKey_KeypadAdd;
        case SDL_SCANCODE_KP_ENTER: return ImGuiKey_KeypadEnter;
        case SDL_SCANCODE_KP_EQUALS: return ImGuiKey_KeypadEqual;
        default: break;
    }
    switch (keycode) {
        case SDLK_TAB: return ImGuiKey_Tab;
        case SDLK_LEFT: return ImGuiKey_LeftArrow;
        case SDLK_RIGHT: return ImGuiKey_RightArrow;
        case SDLK_UP: return ImGuiKey_UpArrow;
        case SDLK_DOWN: return ImGuiKey_DownArrow;
        case SDLK_PAGEUP: return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
        case SDLK_HOME: return ImGuiKey_Home;
        case SDLK_END: return ImGuiKey_End;
        case SDLK_INSERT: return ImGuiKey_Insert;
        case SDLK_DELETE: return ImGuiKey_Delete;
        case SDLK_BACKSPACE: return ImGuiKey_Backspace;
        case SDLK_SPACE: return ImGuiKey_Space;
        case SDLK_RETURN: return ImGuiKey_Enter;
        case SDLK_ESCAPE: return ImGuiKey_Escape;
        case SDLK_COMMA: return ImGuiKey_Comma;
        case SDLK_PERIOD: return ImGuiKey_Period;
        case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
        case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
        case SDLK_SCROLLLOCK: return ImGuiKey_ScrollLock;
        case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
        case SDLK_PRINTSCREEN: return ImGuiKey_PrintScreen;
        case SDLK_PAUSE: return ImGuiKey_Pause;
        case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
        case SDLK_LSHIFT: return ImGuiKey_LeftShift;
        case SDLK_LALT: return ImGuiKey_LeftAlt;
        case SDLK_LGUI: return ImGuiKey_LeftSuper;
        case SDLK_RCTRL: return ImGuiKey_RightCtrl;
        case SDLK_RSHIFT: return ImGuiKey_RightShift;
        case SDLK_RALT: return ImGuiKey_RightAlt;
        case SDLK_RGUI: return ImGuiKey_RightSuper;
        case SDLK_APPLICATION: return ImGuiKey_Menu;
        case SDLK_0: return ImGuiKey_0;
        case SDLK_1: return ImGuiKey_1;
        case SDLK_2: return ImGuiKey_2;
        case SDLK_3: return ImGuiKey_3;
        case SDLK_4: return ImGuiKey_4;
        case SDLK_5: return ImGuiKey_5;
        case SDLK_6: return ImGuiKey_6;
        case SDLK_7: return ImGuiKey_7;
        case SDLK_8: return ImGuiKey_8;
        case SDLK_9: return ImGuiKey_9;
        case SDLK_A: return ImGuiKey_A;
        case SDLK_B: return ImGuiKey_B;
        case SDLK_C: return ImGuiKey_C;
        case SDLK_D: return ImGuiKey_D;
        case SDLK_E: return ImGuiKey_E;
        case SDLK_F: return ImGuiKey_F;
        case SDLK_G: return ImGuiKey_G;
        case SDLK_H: return ImGuiKey_H;
        case SDLK_I: return ImGuiKey_I;
        case SDLK_J: return ImGuiKey_J;
        case SDLK_K: return ImGuiKey_K;
        case SDLK_L: return ImGuiKey_L;
        case SDLK_M: return ImGuiKey_M;
        case SDLK_N: return ImGuiKey_N;
        case SDLK_O: return ImGuiKey_O;
        case SDLK_P: return ImGuiKey_P;
        case SDLK_Q: return ImGuiKey_Q;
        case SDLK_R: return ImGuiKey_R;
        case SDLK_S: return ImGuiKey_S;
        case SDLK_T: return ImGuiKey_T;
        case SDLK_U: return ImGuiKey_U;
        case SDLK_V: return ImGuiKey_V;
        case SDLK_W: return ImGuiKey_W;
        case SDLK_X: return ImGuiKey_X;
        case SDLK_Y: return ImGuiKey_Y;
        case SDLK_Z: return ImGuiKey_Z;
        case SDLK_F1: return ImGuiKey_F1;
        case SDLK_F2: return ImGuiKey_F2;
        case SDLK_F3: return ImGuiKey_F3;
        case SDLK_F4: return ImGuiKey_F4;
        case SDLK_F5: return ImGuiKey_F5;
        case SDLK_F6: return ImGuiKey_F6;
        case SDLK_F7: return ImGuiKey_F7;
        case SDLK_F8: return ImGuiKey_F8;
        case SDLK_F9: return ImGuiKey_F9;
        case SDLK_F10: return ImGuiKey_F10;
        case SDLK_F11: return ImGuiKey_F11;
        case SDLK_F12: return ImGuiKey_F12;
        case SDLK_F13: return ImGuiKey_F13;
        case SDLK_F14: return ImGuiKey_F14;
        case SDLK_F15: return ImGuiKey_F15;
        case SDLK_F16: return ImGuiKey_F16;
        case SDLK_F17: return ImGuiKey_F17;
        case SDLK_F18: return ImGuiKey_F18;
        case SDLK_F19: return ImGuiKey_F19;
        case SDLK_F20: return ImGuiKey_F20;
        case SDLK_F21: return ImGuiKey_F21;
        case SDLK_F22: return ImGuiKey_F22;
        case SDLK_F23: return ImGuiKey_F23;
        case SDLK_F24: return ImGuiKey_F24;
        case SDLK_AC_BACK: return ImGuiKey_AppBack;
        case SDLK_AC_FORWARD: return ImGuiKey_AppForward;
        default: break;
    }

    // Fallback to scancode
    switch (scancode) {
        case SDL_SCANCODE_GRAVE: return ImGuiKey_GraveAccent;
        case SDL_SCANCODE_MINUS: return ImGuiKey_Minus;
        case SDL_SCANCODE_EQUALS: return ImGuiKey_Equal;
        case SDL_SCANCODE_LEFTBRACKET: return ImGuiKey_LeftBracket;
        case SDL_SCANCODE_RIGHTBRACKET: return ImGuiKey_RightBracket;
        case SDL_SCANCODE_NONUSBACKSLASH: return ImGuiKey_Oem102;
        case SDL_SCANCODE_BACKSLASH: return ImGuiKey_Backslash;
        case SDL_SCANCODE_SEMICOLON: return ImGuiKey_Semicolon;
        case SDL_SCANCODE_APOSTROPHE: return ImGuiKey_Apostrophe;
        case SDL_SCANCODE_COMMA: return ImGuiKey_Comma;
        case SDL_SCANCODE_PERIOD: return ImGuiKey_Period;
        case SDL_SCANCODE_SLASH: return ImGuiKey_Slash;
        default: break;
    }
    return ImGuiKey_None;
}

void imgui_update_key_modifiers(uint16_t sdl_key_mods) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (sdl_key_mods & SDL_KMOD_CTRL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (sdl_key_mods & SDL_KMOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (sdl_key_mods & SDL_KMOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (sdl_key_mods & SDL_KMOD_GUI) != 0);
}

// ---------------------------------------------------------------------------
// Event observers（Pulse* 事件 -> ImGuiIO）
// ---------------------------------------------------------------------------

bool event_belongs_to_window(pulse_imgui_plugin_state* state, ecs_world_t* world, ecs_entity_t window) {
    // 事件可能不携带窗口（window == 0），此时不过滤。
    ecs_entity_t target = imgui_get_window_entity(world, state);
    return !window || !target || window == target;
}

void imgui_key_event_observer(ecs_iter_t* it) {
    pulse_imgui_plugin_state* state =
        static_cast<pulse_imgui_plugin_state*>(it->ctx);
    const PulseKeyEvent* evt = static_cast<const PulseKeyEvent*>(it->param);
    if (!state || !evt || !state->context) {
        return;
    }
    if (!event_belongs_to_window(state, it->world, evt->window)) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();

    imgui_update_key_modifiers(evt->mod);
    ImGuiKey key = imgui_sdl3_key_event_to_imgui_key(
        (SDL_Keycode)evt->keycode,
        (SDL_Scancode)evt->scancode);
    if (key != ImGuiKey_None) {
        io.AddKeyEvent(key, evt->pressed);
        io.SetKeyEventNativeData(key, evt->keycode, evt->scancode, evt->scancode);
    }
}

void imgui_mouse_button_observer(ecs_iter_t* it) {
    pulse_imgui_plugin_state* state =
        static_cast<pulse_imgui_plugin_state*>(it->ctx);
    const PulseMouseButtonEvent* evt =
        static_cast<const PulseMouseButtonEvent*>(it->param);
    if (!state || !evt || !state->context) {
        return;
    }
    if (!event_belongs_to_window(state, it->world, evt->window)) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();
    // imgui 鼠标按键从 0 开始；SDL 从 1 开始。
    if (evt->button >= 1 && evt->button <= 5) {
        int button = evt->button - 1;
        io.AddMouseSourceEvent(
            evt->is_touch ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
        io.AddMouseButtonEvent(button, evt->pressed);

        pulse_imgui_platform_data* pd = platform_data_from_imgui_context();
        if (pd) {
            if (evt->pressed) {
                pd->mouse_buttons_down |= (1 << button);
            } else {
                pd->mouse_buttons_down &= ~(1 << button);
            }
        }
    }
    io.AddMousePosEvent(evt->x, evt->y);
}

void imgui_mouse_scroll_observer(ecs_iter_t* it) {
    pulse_imgui_plugin_state* state =
        static_cast<pulse_imgui_plugin_state*>(it->ctx);
    const PulseMouseScrollEvent* evt =
        static_cast<const PulseMouseScrollEvent*>(it->param);
    if (!state || !evt || !state->context) {
        return;
    }
    if (!event_belongs_to_window(state, it->world, evt->window)) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseSourceEvent(
        evt->is_touch ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
    // 与官方 imgui_impl_sdl3 保持一致：SDL 横向滚轮与 imgui 符号相反。
    io.AddMouseWheelEvent(-evt->x, evt->y);
}

void imgui_text_input_observer(ecs_iter_t* it) {
    pulse_imgui_plugin_state* state =
        static_cast<pulse_imgui_plugin_state*>(it->ctx);
    const PulseTextInputEvent* evt =
        static_cast<const PulseTextInputEvent*>(it->param);
    if (!state || !evt || !state->context) {
        return;
    }
    if (!event_belongs_to_window(state, it->world, evt->window)) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(evt->text);
}

void imgui_window_focus_observer(ecs_iter_t* it) {
    pulse_imgui_plugin_state* state =
        static_cast<pulse_imgui_plugin_state*>(it->ctx);
    const PulseWindowFocusEvent* evt =
        static_cast<const PulseWindowFocusEvent*>(it->param);
    if (!state || !evt || !state->context) {
        return;
    }
    if (!event_belongs_to_window(state, it->world, evt->window)) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(evt->focused);
}

void imgui_window_mouse_hover_observer(ecs_iter_t* it) {
    pulse_imgui_plugin_state* state =
        static_cast<pulse_imgui_plugin_state*>(it->ctx);
    const PulseWindowMouseHoverEvent* evt =
        static_cast<const PulseWindowMouseHoverEvent*>(it->param);
    if (!state || !evt || !state->context) {
        return;
    }
    if (!event_belongs_to_window(state, it->world, evt->window)) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    pulse_imgui_platform_data* pd = platform_data_from_imgui_context();
    if (evt->entered) {
        if (pd) {
            pd->mouse_pending_leave_frame = 0;
        }
    } else {
        if (pd) {
            pd->mouse_pending_leave_frame = ImGui::GetFrameCount() + 1;
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Platform backend lifecycle
// ---------------------------------------------------------------------------

bool imgui_platform_init(ecs_world_t* world, pulse_imgui_plugin_state* state) {
    if (!world || !state || !state->context) {
        return false;
    }

    ecs_entity_t window_entity = imgui_get_window_entity(world, state);
    const PulseSdlWindow* sdl_window =
        window_entity ? ecs_get(world, window_entity, PulseSdlWindow) : nullptr;
    if (!sdl_window || !sdl_window->handle) {
        return false;
    }

    auto* pd = new pulse_imgui_platform_data();
    pd->state = state;
    pd->primary_window = window_entity;
    pd->window = sdl_window->handle;
    pd->window_id = sdl_window->window_id;

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "pulse_imgui-sdl3";
    io.BackendPlatformUserData = pd;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_GetClipboardTextFn = imgui_platform_get_clipboard_text;
    platform_io.Platform_SetClipboardTextFn = imgui_platform_set_clipboard_text;
    platform_io.Platform_SetImeDataFn = imgui_platform_set_ime_data;
    platform_io.Platform_OpenInShellFn = imgui_platform_open_in_shell;

    pd->mouse_cursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    pd->mouse_cursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    pd->mouse_cursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    pd->mouse_cursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    pd->mouse_cursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    pd->mouse_cursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE);
    pd->mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
    pd->mouse_cursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    pd->mouse_cursors[ImGuiMouseCursor_Wait] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    pd->mouse_cursors[ImGuiMouseCursor_Progress] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_PROGRESS);
    pd->mouse_cursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)(intptr_t)pd->window_id;

    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");

    state->platform_data = pd;
    return true;
}

void imgui_platform_new_frame(pulse_imgui_plugin_state* state) {
    if (!state || !state->context) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();
    pulse_imgui_platform_data* pd = platform_data_from_imgui_context();
    if (!pd) {
        return;
    }

    if (io.WantSetMousePos && pd->window) {
        SDL_WarpMouseInWindow(pd->window, io.MousePos.x, io.MousePos.y);
    }

    // 鼠标离开窗口后延迟一帧清掉 hover（与官方 imgui_impl_sdl3 一致）。
    if (pd->mouse_pending_leave_frame &&
        pd->mouse_pending_leave_frame >= ImGui::GetFrameCount() &&
        pd->mouse_buttons_down == 0) {
        pd->mouse_pending_leave_frame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    imgui_platform_update_mouse_cursor();
}

void imgui_platform_shutdown(pulse_imgui_plugin_state* state) {
    if (!state || !state->context) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();
    auto* pd = static_cast<pulse_imgui_platform_data*>(io.BackendPlatformUserData);
    if (!pd) {
        state->platform_data = nullptr;
        return;
    }

    if (pd->ime_window) {
        SDL_StopTextInput(pd->ime_window);
        pd->ime_window = nullptr;
    }
    if (pd->clipboard_text) {
        SDL_free(pd->clipboard_text);
        pd->clipboard_text = nullptr;
    }
    for (SDL_Cursor* cursor : pd->mouse_cursors) {
        if (cursor) {
            SDL_DestroyCursor(cursor);
        }
    }

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_GetClipboardTextFn = nullptr;
    platform_io.Platform_SetClipboardTextFn = nullptr;
    platform_io.Platform_SetImeDataFn = nullptr;
    platform_io.Platform_OpenInShellFn = nullptr;

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos);

    delete pd;
    state->platform_data = nullptr;
}

void install_imgui_input(ecs_world_t* world, pulse_imgui_plugin_state* state) {
    if (!world || !state) {
        return;
    }

    // 键盘 / 鼠标按键 / 滚轮事件发到 pulse_input 单例实体。
    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseKeyboardInput);
        desc.events[0] = ecs_id(PulseKeyEvent);
        desc.callback = imgui_key_event_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }
    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseMouseInput);
        desc.events[0] = ecs_id(PulseMouseButtonEvent);
        desc.callback = imgui_mouse_button_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }
    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseMouseScroll);
        desc.events[0] = ecs_id(PulseMouseScrollEvent);
        desc.callback = imgui_mouse_scroll_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }

    // 文本输入 / 窗口焦点 / 鼠标进入离开事件发到窗口实体（事件数据含 window 字段）。
    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseWindow);
        desc.events[0] = ecs_id(PulseTextInputEvent);
        desc.callback = imgui_text_input_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }
    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseWindow);
        desc.events[0] = ecs_id(PulseWindowFocusEvent);
        desc.callback = imgui_window_focus_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }
    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseWindow);
        desc.events[0] = ecs_id(PulseWindowMouseHoverEvent);
        desc.callback = imgui_window_mouse_hover_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }
}

} // namespace pulse_imgui_internal
