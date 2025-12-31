#ifndef BACKEND_H
#define BACKEND_H

#ifdef _WIN32
#include <windows.h>

static void backend_set_window_icon(RGFW_window* win) {
    if (!win) return;
    HWND hwnd = (HWND)RGFW_window_getHWND(win);
    // 101 is the resource ID we defined in nob.c
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
}

// Returns costume ID (1-9) if hotkey pressed, 0 otherwise
static int GetConfiguredHotkey(const int* keyBindings, int count) {
    for (int i = 0; i < count; i++) {
        if (keyBindings[i] > 0 && (GetAsyncKeyState(keyBindings[i]) & 1)) {
            return i + 1;
        }
    }
    return 0;
}

#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static Display* g_x11Display = NULL;
static int g_x11Initialized = 0;
static char g_prevKeyState[32] = {0};

// Convert Windows VK code / ASCII to X11 KeySym
static KeySym VKToKeySym(int vk) {
    if (vk >= 'A' && vk <= 'Z') {
        return XK_a + (vk - 'A');
    }
    if (vk >= '0' && vk <= '9') {
        return XK_0 + (vk - '0');
    }
    // Numpad 0-9 (Windows VK: 0x60-0x69)
    if (vk >= 0x60 && vk <= 0x69) {
        return XK_KP_0 + (vk - 0x60);
    }
    // Function keys F1-F24 (Windows VK: 0x70-0x87)
    if (vk >= 0x70 && vk <= 0x87) {
        return XK_F1 + (vk - 0x70);
    }
    return NoSymbol;
}

static int GetConfiguredHotkey(const int* keyBindings, int count) {
    if (!g_x11Initialized) {
        g_x11Display = XOpenDisplay(NULL);
        g_x11Initialized = 1;
    }
    if (!g_x11Display) return 0;

    char keyState[32];
    XQueryKeymap(g_x11Display, keyState);

    for (int i = 0; i < count; i++) {
        if (keyBindings[i] <= 0) continue;

        KeySym keysym = VKToKeySym(keyBindings[i]);
        if (keysym == NoSymbol) continue;

        KeyCode keycode = XKeysymToKeycode(g_x11Display, keysym);
        if (keycode == 0) continue;

        int byteIndex = keycode / 8;
        int bitIndex = keycode % 8;

        int isPressed = (keyState[byteIndex] & (1 << bitIndex)) != 0;
        int wasPressed = (g_prevKeyState[byteIndex] & (1 << bitIndex)) != 0;

        if (isPressed && !wasPressed) {
            memcpy(g_prevKeyState, keyState, sizeof(g_prevKeyState));
            return i + 1;
        }
    }

    memcpy(g_prevKeyState, keyState, sizeof(g_prevKeyState));
    return 0;
}

static void backend_set_window_icon(RGFW_window* win) {
    (void)win;
}

static void backend_cleanup_x11(void) {
    if (g_x11Display) {
        XCloseDisplay(g_x11Display);
        g_x11Display = NULL;
    }
}

#else
// macOS or other platforms
static void backend_set_window_icon(RGFW_window* win) {
    (void)win;
}

static int GetConfiguredHotkey(const int* keyBindings, int count) {
    (void)keyBindings;
    (void)count;
    return 0;
}

static void backend_cleanup_x11(void) {}
#endif

#endif
