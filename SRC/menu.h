#ifndef MENU_H
#define MENU_H

#include "shared.h"

// Static state for menu
static Button btnSpawnsettings = { 10, 10, 180, 40, "Settings" };
static Button btnQuit = { 10, 60, 180, 40, "Quit" };

void menu_init(int x, int y) {
    // Menu height increased for 2 buttons
    menu = RGFW_createWindow("Menu", x, y, 200, 110, RGFW_windowOpenGL | RGFW_windowNoBorder);

    if (menu) {
        backend_set_window_icon(menu);
        RGFW_window_makeCurrentContext_OpenGL(menu);
        gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL);
        vg_menu = nvglCreate(NVGL_DEBUG);
        nvgCreateFontMem(vg_menu, "sans", font_data, font_data_len, 0);
        RGFW_window_swapInterval_OpenGL(menu, 0);
    }
}

void menu_handle_event(RGFW_event* event) {
    if (event->type == RGFW_mousePosChanged) {
        btnSpawnsettings.hovered = isPointInRect(event->mouse.x, event->mouse.y, btnSpawnsettings.x, btnSpawnsettings.y, btnSpawnsettings.w, btnSpawnsettings.h);
        btnQuit.hovered = isPointInRect(event->mouse.x, event->mouse.y, btnQuit.x, btnQuit.y, btnQuit.w, btnQuit.h);

        if (btnSpawnsettings.hovered || btnQuit.hovered) {
            RGFW_window_setMouseStandard(menu, RGFW_mousePointingHand);
        } else {
            RGFW_window_setMouseStandard(menu, RGFW_mouseNormal);
        }
    }
    if (event->type == RGFW_mouseButtonPressed) {
        if (btnSpawnsettings.hovered) {
            if (settings) {
                RGFW_window_makeCurrentContext_OpenGL(settings);
                if (vg_settings) nvglDelete(vg_settings);
                RGFW_window_close(settings);
                settings = NULL; vg_settings = NULL;
            }
            settings_init();
            
            // Close menu
            RGFW_window_makeCurrentContext_OpenGL(menu);
            if (vg_menu) nvglDelete(vg_menu);
            RGFW_window_close(menu);
            menu = NULL; vg_menu = NULL;
        } else if (btnQuit.hovered) {
            running = 0;
        }
    }
    if (event->type == RGFW_focusOut) {
        RGFW_window_makeCurrentContext_OpenGL(menu);
        if (vg_menu) nvglDelete(vg_menu);
        RGFW_window_close(menu);
        menu = NULL; vg_menu = NULL;
    }
}

void menu_draw(void) {
    if (!menu) return;
    RGFW_window_makeCurrentContext_OpenGL(menu);
    glViewport(0, 0, menu->w, menu->h);
    glClearColor(0.357f, 0.808f, 0.980f, 1.0f); // Blue
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    nvgBeginFrame(vg_menu, (float)menu->w, (float)menu->h, 1.0f);
    drawButton(vg_menu, &btnSpawnsettings);
    drawButton(vg_menu, &btnQuit);
    nvgEndFrame(vg_menu);
    glFlush();
    RGFW_window_swapBuffers_OpenGL(menu);
}

#endif
