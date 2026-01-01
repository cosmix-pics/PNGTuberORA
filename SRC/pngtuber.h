#ifndef PNGTUBER_H
#define PNGTUBER_H

#include "shared.h"
#include "ora_loader.h"
#include <stdio.h>

void pngtuber_init(void) {
    RGFW_getGlobalHints_OpenGL()->major = 3;
    RGFW_getGlobalHints_OpenGL()->minor = 3;
    RGFW_getGlobalHints_OpenGL()->stencil = 8;
    RGFW_getGlobalHints_OpenGL()->doubleBuffer = 1;
    RGFW_getGlobalHints_OpenGL()->alpha = 8;

    win = RGFW_createWindow("PNGTuberORA", 100, 100, 800, 600, RGFW_windowOpenGL | RGFW_windowTransparent);
    if (!win) {
        printf("Failed to create main window\n");
        running = 0;
        return;
    }
    backend_set_window_icon(win);

    RGFW_window_makeCurrentContext_OpenGL(win);
    if (!gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL)) {
        printf("Failed to load GLAD\n");
        running = 0;
        return;
    }

    vg_win = nvglCreate(NVGL_DEBUG);
    if (!vg_win) {
        printf("[Error] Failed to create NanoVG context for main window\n");
        RGFW_window_close(win);
        win = NULL;
        running = 0;
        return;
    }
    nvgCreateFontMem(vg_win, "sans", font_data, font_data_len, 0);

    RGFW_window_swapInterval_OpenGL(win, 0);
}

void pngtuber_handle_event(RGFW_event* event) {
    if (event->type == RGFW_quit) {
        running = 0;
        return;
    }

    if (event->type == RGFW_mouseButtonPressed) {
        if (event->button.value == RGFW_mouseRight) {
            // Open menu window at mouse position
            i32 mx, my;
            RGFW_window_getMouse(win, &mx, &my);
            if (!menu) {
                menu_show(mx, my);
            }
        } else if (event->button.value == RGFW_mouseLeft) {
            // Close menu if clicking main window
            if (menu) {
                menu_hide();
            }
        }
    }
}

void pngtuber_draw(void) {
    if (!win || !vg_win) return;
    RGFW_window_makeCurrentContext_OpenGL(win);
    glViewport(0, 0, win->w, win->h);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Transparent background
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    nvgBeginFrame(vg_win, (float)win->w, (float)win->h, 1.0f);

    if (g_avatar.isLoaded) {
        // Draw the avatar
        DrawAvatar(vg_win, &g_avatar, (float)win->w, (float)win->h);
    } else {
        // Show instructions when no avatar loaded
        nvgBeginPath(vg_win);
        nvgFillColor(vg_win, ui_text_color);
        nvgFontSize(vg_win, 20.0f);
        nvgFontFace(vg_win, "sans");
        nvgTextAlign(vg_win, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgText(vg_win, 10, 10, "Right click to open menu", NULL);
        nvgText(vg_win, 10, 35, "Load an ORA file from Settings", NULL);
    }

    nvgEndFrame(vg_win);
    glFlush();
    RGFW_window_swapBuffers_OpenGL(win);
}

#endif
