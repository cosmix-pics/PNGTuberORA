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
#else
static void backend_set_window_icon(RGFW_window* win) {
    (void)win;
    // Do nothing on Linux/macOS as requested
}
#endif

#endif
