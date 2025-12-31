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

// Font data (generated in build/font.h)
extern unsigned char font_data[];
extern unsigned int font_data_len;

// Window Management State
extern RGFW_window* win;
extern RGFW_window* menu;
extern RGFW_window* settings;
extern RGFW_window* dialog;

extern NVGcontext* vg_win;
extern NVGcontext* vg_menu;
extern NVGcontext* vg_settings;
extern NVGcontext* vg_dialog;

extern int running;

// Handlers for specific windows
void pngtuber_init(void);
void pngtuber_draw(void);
void pngtuber_handle_event(RGFW_event* event);

void menu_init(int x, int y);
void menu_draw(void);
void menu_handle_event(RGFW_event* event);

void settings_init(void);
void settings_draw(void);
void settings_handle_event(RGFW_event* event);

void dialog_init(void);
void dialog_draw(void);
void dialog_handle_event(RGFW_event* event);

#endif
