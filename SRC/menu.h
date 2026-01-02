#ifndef MENU_H
#define MENU_H

#include "shared.h"

// Menu window dimensions
#define MENU_WIDTH 200
#define MENU_HEIGHT 110

// Menu button state
static Button btnMenuSettings = { 10, 10, 180, 40, "Settings", 0 };
static Button btnMenuQuit = { 10, 60, 180, 40, "Quit", 0 };

void menu_show(int x, int y) {
    if (menu) return;  // Already open

    // Deactivate current GL context before creating new window
    DeactivateCurrentGLContext();

    // Create menu window at screen position (x, y is relative to main window)
    // Add main window position to get screen coordinates
    int screenX = win->x + x;
    int screenY = win->y + y;

    menu = RGFW_createWindow("Menu", screenX, screenY, MENU_WIDTH, MENU_HEIGHT,
                             RGFW_windowOpenGL | RGFW_windowNoResize | RGFW_windowNoBorder);
    if (!menu) {
        printf("[Error] Failed to create menu window\n");
        return;
    }

    RGFW_window_makeCurrentContext_OpenGL(menu);
    gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL);

    vg_menu = nvglCreate(NVGL_DEBUG);
    if (!vg_menu) {
        printf("[Error] Failed to create NanoVG context for menu\n");
        RGFW_window_close(menu);
        menu = NULL;
        return;
    }

    nvgCreateFontMem(vg_menu, "sans", font_data, font_data_len, 0);
    RGFW_window_swapInterval_OpenGL(menu, 1);

    g_menuVisible = 1;
}

void menu_hide(void) {
    if (menu) {
        RGFW_window_makeCurrentContext_OpenGL(menu);
        if (vg_menu) {
            nvglDelete(vg_menu);
            vg_menu = NULL;
        }
        RGFW_window_close(menu);
        menu = NULL;
    }
    g_menuVisible = 0;
}

// Returns 1 if menu consumed the event
int menu_handle_event(RGFW_event* event) {
    if (!menu || !vg_menu) return 0;

    if (event->type == RGFW_quit) {
        menu_hide();
        return 1;
    }

    // Get mouse position relative to menu window
    i32 mx, my;
    RGFW_window_getMouse(menu, &mx, &my);

    if (event->type == RGFW_mousePosChanged) {
        btnMenuSettings.hovered = isPointInRect((float)mx, (float)my,
            btnMenuSettings.x, btnMenuSettings.y, btnMenuSettings.w, btnMenuSettings.h);
        btnMenuQuit.hovered = isPointInRect((float)mx, (float)my,
            btnMenuQuit.x, btnMenuQuit.y, btnMenuQuit.w, btnMenuQuit.h);
        return 1;
    }

    if (event->type == RGFW_mouseButtonPressed && event->button.value == RGFW_mouseLeft) {
        if (btnMenuSettings.hovered) {
            menu_hide();
            settings_close();
            dialog_close();
            settings_init();
            return 1;
        } else if (btnMenuQuit.hovered) {
            running = 0;
            return 1;
        }
    }

    if (event->type == RGFW_focusOut) {
        menu_hide();
        return 1;
    }

    return 0;
}

void menu_draw(void) {
    if (!menu || !vg_menu) return;

    RGFW_window_makeCurrentContext_OpenGL(menu);
    glViewport(0, 0, menu->w, menu->h);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg_menu, (float)menu->w, (float)menu->h, 1.0f);

    // Draw menu background
    nvgBeginPath(vg_menu);
    nvgRoundedRect(vg_menu, 0, 0, (float)MENU_WIDTH, (float)MENU_HEIGHT, 5.0f);
    nvgFillColor(vg_menu, ui_button_active); // Use active color for menu bg
    nvgFill(vg_menu);

    // Draw border
    nvgBeginPath(vg_menu);
    nvgRoundedRect(vg_menu, 0, 0, (float)MENU_WIDTH, (float)MENU_HEIGHT, 5.0f);
    nvgStrokeColor(vg_menu, ui_button_border);
    nvgStrokeWidth(vg_menu, 2.0f);
    nvgStroke(vg_menu);

    // Draw buttons
    drawButton(vg_menu, &btnMenuSettings);
    drawButton(vg_menu, &btnMenuQuit);

    nvgEndFrame(vg_menu);
    glFlush();
    RGFW_window_swapBuffers_OpenGL(menu);
}

#endif
