#ifndef SETTINGS_H
#define SETTINGS_H

#include "shared.h"

static Button btnBrowse = { 20, 50, 460, 40, "Browse..." };
static Slider sliderVolume = { 20, 140, 460, 20, 0.5f, 0 }; // Height 20 for knob radius 10

void settings_init(void) {
    settings = RGFW_createWindow("Settings", 400, 600, 500, 600, RGFW_windowOpenGL | RGFW_windowNoResize);
    if (settings) {
        backend_set_window_icon(settings);
        RGFW_window_makeCurrentContext_OpenGL(settings);
        gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL);
        vg_settings = nvglCreate(NVGL_DEBUG);
        nvgCreateFontMem(vg_settings, "sans", font_data, font_data_len, 0);
        RGFW_window_swapInterval_OpenGL(settings, 0);
    }
}

void settings_handle_event(RGFW_event* event) {
    if (event->type == RGFW_quit) {
        if (dialog) {
            RGFW_window_makeCurrentContext_OpenGL(dialog);
            if (vg_dialog) nvglDelete(vg_dialog);
            RGFW_window_close(dialog);
            dialog = NULL; vg_dialog = NULL;
        }
        RGFW_window_makeCurrentContext_OpenGL(settings);
        if (vg_settings) nvglDelete(vg_settings);
        RGFW_window_close(settings);
        settings = NULL; vg_settings = NULL;
        return;
    }
    
    if (event->type == RGFW_mousePosChanged) {
        btnBrowse.hovered = isPointInRect(event->mouse.x, event->mouse.y, btnBrowse.x, btnBrowse.y, btnBrowse.w, btnBrowse.h);
        int sliderHover = isPointInRect(event->mouse.x, event->mouse.y, sliderVolume.x, sliderVolume.y - 5, sliderVolume.w, sliderVolume.h + 10);

        if (btnBrowse.hovered || sliderHover || sliderVolume.dragging) {
            RGFW_window_setMouseStandard(settings, RGFW_mousePointingHand);
        } else {
            RGFW_window_setMouseStandard(settings, RGFW_mouseNormal);
        }
        
        if (sliderVolume.dragging) {
            float val = (event->mouse.x - sliderVolume.x) / sliderVolume.w;
            if (val < 0.0f) val = 0.0f;
            if (val > 1.0f) val = 1.0f;
            sliderVolume.value = val;
        }
    }
    
    if (event->type == RGFW_mouseButtonPressed) {
        if (event->button.value == RGFW_mouseLeft) {
            i32 mx, my;
            RGFW_window_getMouse(settings, &mx, &my);

            if (btnBrowse.hovered) { // hover state might be stale if mouse didn't move
                 // Re-check hover with current mouse pos just in case
                 if (isPointInRect(mx, my, btnBrowse.x, btnBrowse.y, btnBrowse.w, btnBrowse.h)) {
                    if (dialog) {
                        RGFW_window_makeCurrentContext_OpenGL(dialog);
                        if (vg_dialog) nvglDelete(vg_dialog);
                        RGFW_window_close(dialog);
                        dialog = NULL; vg_dialog = NULL;
                    }
                    dialog_init();
                 }
            }
            
            // Check slider
            if (isPointInRect(mx, my, sliderVolume.x, sliderVolume.y - 5, sliderVolume.w, sliderVolume.h + 10)) {
                sliderVolume.dragging = 1;
                float val = (mx - sliderVolume.x) / sliderVolume.w;
                if (val < 0.0f) val = 0.0f;
                if (val > 1.0f) val = 1.0f;
                sliderVolume.value = val;
            }
        }
    }
    
    if (event->type == RGFW_mouseButtonReleased) {
        sliderVolume.dragging = 0;
    }
}

void settings_draw(void) {
    if (!settings) return;
    RGFW_window_makeCurrentContext_OpenGL(settings);
    glViewport(0, 0, settings->w, settings->h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    nvgBeginFrame(vg_settings, (float)settings->w, (float)settings->h, 1.0f);
    
    drawLabel(vg_settings, "Avatar File:", 20, 40); // Baseline y
    drawButton(vg_settings, &btnBrowse);
    
    drawLabel(vg_settings, "Volume Threshold:", 20, 130);
    drawSlider(vg_settings, &sliderVolume);
    
    nvgEndFrame(vg_settings);
    glFlush();
    RGFW_window_swapBuffers_OpenGL(settings);
}

#endif
