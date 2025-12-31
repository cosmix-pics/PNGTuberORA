#ifndef SHARED_H
#define SHARED_H

// Core Macros
#define RGFW_OPENGL

#include "RGFW.h"
#include "backend.h"
#include "glad/glad.h"
#include "nanovgXC/nanovg.h"
#include "nanovgXC/nanovg_gl.h"
#include "UI.h"
#include "avatar.h"
#include "config.h"

// Font data (generated in build/font.h)
extern unsigned char font_data[];
extern unsigned int font_data_len;

// Helper to deactivate current OpenGL context before creating new windows
// This fixes RGFW multi-window crash on Windows
static inline void DeactivateCurrentGLContext(void) {
#ifdef _WIN32
    wglMakeCurrent(NULL, NULL);
#elif defined(__linux__)
    // For X11, need the display - but RGFW handles this internally
    // Just ensure no context is current
#endif
}

// Window Management State
extern RGFW_window* win;
extern NVGcontext* vg_win;
extern int running;

// Settings window
extern RGFW_window* settings;
extern NVGcontext* vg_settings;

// Dialog window
extern RGFW_window* dialog;
extern NVGcontext* vg_dialog;

// Menu window
extern RGFW_window* menu;
extern NVGcontext* vg_menu;
extern int g_menuVisible;

// Global application state
extern AppConfig g_config;
extern Avatar g_avatar;
extern char g_configPath[1024];
extern char g_visemePath[1024];
extern char g_dialogPath[512];

// Handlers for main window
void pngtuber_init(void);
void pngtuber_draw(void);
void pngtuber_handle_event(RGFW_event* event);

// Menu window functions
void menu_show(int x, int y);
void menu_hide(void);
void menu_draw(void);
int menu_handle_event(RGFW_event* event);

// Settings window functions
void settings_init(void);
void settings_close(void);
void settings_draw(void);
void settings_handle_event(RGFW_event* event);

// Dialog window functions
void dialog_init(void);
void dialog_close(void);
void dialog_draw(void);
void dialog_handle_event(RGFW_event* event);
void dialog_set_path(const char* path);

#endif
