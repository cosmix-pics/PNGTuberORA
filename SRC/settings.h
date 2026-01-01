#ifndef SETTINGS_H
#define SETTINGS_H

#include "shared.h"
#include "config.h"
#include "viseme.h"
#include <stdio.h>

// Settings window dimensions
#define SETTINGS_WIDTH 600
#define SETTINGS_HEIGHT 640

// UI State
static Slider sliderVolume = { 20, 50, 560, 20, 0.15f, 0 };
static TextDisplay txtModelPath = { 20, 120, 410, 28, "" };
static Button btnBrowse = { 440, 120, 140, 28, "Browse...", 0 };
static Button btnTheme = { 400, 10, 180, 28, "Theme: Trans", 0 };

// Viseme training
static ProgressBar visBars[4];
static Button visButtons[5];  // None, Silence, CH, OU, AA
static Button btnSaveViseme = { 500, 350, 80, 28, "Save", 0 };
static int currentTrainingSlot = -1;

// Hotkey buttons
static HotkeyButton hotkeyBtns[MAX_HOTKEYS];
static int waitingForKeyIdx = -1;

static int settings_initialized = 0;

static void UpdateThemeButtonText(void) {
    switch (g_config.theme) {
        case THEME_TRANS: btnTheme.text = "Theme: Trans"; break;
        case THEME_DARK:  btnTheme.text = "Theme: Dark"; break;
        case THEME_WHITE: btnTheme.text = "Theme: White"; break;
    }
}

void settings_init(void) {
    // Deactivate current GL context before creating new window
    DeactivateCurrentGLContext();

    settings = RGFW_createWindow("Settings", 100, 100, SETTINGS_WIDTH, SETTINGS_HEIGHT,
                                  RGFW_windowOpenGL | RGFW_windowNoResize);
    if (!settings) {
        printf("[Error] Failed to create settings window\n");
        return;
    }

    backend_set_window_icon(settings);
    RGFW_window_makeCurrentContext_OpenGL(settings);
    gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL);

    vg_settings = nvglCreate(NVGL_DEBUG);
    if (!vg_settings) {
        printf("[Error] Failed to create NanoVG context for settings\n");
        RGFW_window_close(settings);
        settings = NULL;
        return;
    }

    nvgCreateFontMem(vg_settings, "sans", font_data, font_data_len, 0);
    RGFW_window_swapInterval_OpenGL(settings, 1);
    
    // Apply current theme
    SetTheme(g_config.theme);
    UpdateThemeButtonText();

    // Initialize UI state from config
    sliderVolume.value = g_config.voiceThreshold;
    strncpy(txtModelPath.text, g_config.defaultModelPath, 511);

    // Initialize viseme progress bars
    for (int i = 0; i < 4; i++) {
        visBars[i].x = 120;
        visBars[i].y = 210 + i * 32;
        visBars[i].w = 440;
        visBars[i].h = 18;
        visBars[i].value = 0;
    }

    // Viseme training buttons
    const char* btnNames[] = { "None", "Silence", "CH", "OU", "AA" };
    for (int i = 0; i < 5; i++) {
        visButtons[i].x = 20 + i * 90;
        visButtons[i].y = 350;
        visButtons[i].w = 80;
        visButtons[i].h = 28;
        visButtons[i].text = btnNames[i];
        visButtons[i].hovered = 0;
    }

    // Hotkey buttons (3x3 grid)
    for (int i = 0; i < MAX_HOTKEYS; i++) {
        hotkeyBtns[i].x = 100 + (i % 3) * 170;
        hotkeyBtns[i].y = 450 + (i / 3) * 55;
        hotkeyBtns[i].w = 80;
        hotkeyBtns[i].h = 32;
        hotkeyBtns[i].keyCode = g_config.hotkeys[i];
        hotkeyBtns[i].waitingForKey = 0;
        hotkeyBtns[i].hovered = 0;
    }

    settings_initialized = 1;
}

void settings_close(void) {
    if (settings) {
        RGFW_window_makeCurrentContext_OpenGL(settings);
        if (vg_settings) {
            nvglDelete(vg_settings);
            vg_settings = NULL;
        }
        RGFW_window_close(settings);
        settings = NULL;
    }
    waitingForKeyIdx = -1;
    currentTrainingSlot = -1;
    VisemeSetTraining(-1);
}

void settings_handle_event(RGFW_event* event) {
    if (!settings || !vg_settings) return;

    if (event->type == RGFW_quit) {
        settings_close();
        return;
    }

    i32 mx, my;
    RGFW_window_getMouse(settings, &mx, &my);

    if (event->type == RGFW_mousePosChanged) {
        // Update hover states
        btnBrowse.hovered = isPointInRect((float)mx, (float)my, btnBrowse.x, btnBrowse.y, btnBrowse.w, btnBrowse.h);
        btnTheme.hovered = isPointInRect((float)mx, (float)my, btnTheme.x, btnTheme.y, btnTheme.w, btnTheme.h);
        btnSaveViseme.hovered = isPointInRect((float)mx, (float)my, btnSaveViseme.x, btnSaveViseme.y, btnSaveViseme.w, btnSaveViseme.h);
        
        for (int i = 0; i < 5; i++) {
            visButtons[i].hovered = isPointInRect((float)mx, (float)my, visButtons[i].x, visButtons[i].y, visButtons[i].w, visButtons[i].h);
        }

        for (int i = 0; i < MAX_HOTKEYS; i++) {
            hotkeyBtns[i].hovered = isPointInRect((float)mx, (float)my, hotkeyBtns[i].x, hotkeyBtns[i].y, hotkeyBtns[i].w, hotkeyBtns[i].h);
        }

        // Slider dragging
        if (sliderVolume.dragging) {
            float val = ((float)mx - sliderVolume.x) / sliderVolume.w;
            if (val < 0.0f) val = 0.0f;
            if (val > 1.0f) val = 1.0f;
            sliderVolume.value = val;
            g_config.voiceThreshold = val;
        }
    }

    if (event->type == RGFW_mouseButtonPressed && event->button.value == RGFW_mouseLeft) {
        // Theme button
        if (btnTheme.hovered) {
            g_config.theme = (g_config.theme + 1) % 3;
            SetTheme(g_config.theme);
            UpdateThemeButtonText();
            SaveConfig(g_configPath, &g_config);
            return;
        }

        // Browse button
        if (btnBrowse.hovered) {
            dialog_init();
            return;
        }

        // Slider
        if (isPointInRect((float)mx, (float)my, sliderVolume.x, sliderVolume.y - 5, sliderVolume.w, sliderVolume.h + 10)) {
            sliderVolume.dragging = 1;
            float val = ((float)mx - sliderVolume.x) / sliderVolume.w;
            if (val < 0.0f) val = 0.0f;
            if (val > 1.0f) val = 1.0f;
            sliderVolume.value = val;
            g_config.voiceThreshold = val;
        }

        // Viseme training buttons
        int visSlots[] = { -1, VIS_SLOT_SILENCE, VIS_SLOT_CH, VIS_SLOT_OU, VIS_SLOT_AA };
        for (int i = 0; i < 5; i++) {
            if (visButtons[i].hovered) {
                currentTrainingSlot = visSlots[i];
                VisemeSetTraining(currentTrainingSlot);
                return;
            }
        }

        // Save viseme button
        if (btnSaveViseme.hovered) {
            VisemeSave(g_visemePath);
            return;
        }

        // Hotkey buttons
        for (int i = 0; i < MAX_HOTKEYS; i++) {
            if (hotkeyBtns[i].hovered) {
                // Clear previous waiting state
                if (waitingForKeyIdx >= 0) {
                    hotkeyBtns[waitingForKeyIdx].waitingForKey = 0;
                }
                waitingForKeyIdx = i;
                hotkeyBtns[i].waitingForKey = 1;
                return;
            }
        }
    }

    if (event->type == RGFW_mouseButtonReleased) {
        if (sliderVolume.dragging) {
            sliderVolume.dragging = 0;
            SaveConfig(g_configPath, &g_config);
        }
    }

    // Handle key input for hotkey assignment
    if (event->type == RGFW_keyPressed && waitingForKeyIdx >= 0) {
        int key = event->key.value;
        // Convert to VK-style code
        int vk = 0;
        if (key >= RGFW_0 && key <= RGFW_9) {
            vk = '0' + (key - RGFW_0);
        } else if (key >= RGFW_a && key <= RGFW_z) {
            vk = 'A' + (key - RGFW_a);
        } else if (key >= RGFW_F1 && key <= RGFW_F12) {
            vk = 0x70 + (key - RGFW_F1);
        }

        if (vk > 0) {
            hotkeyBtns[waitingForKeyIdx].keyCode = vk;
            g_config.hotkeys[waitingForKeyIdx] = vk;
            SaveConfig(g_configPath, &g_config);
        }
        hotkeyBtns[waitingForKeyIdx].waitingForKey = 0;
        waitingForKeyIdx = -1;
    }
}

void settings_draw(void) {
    if (!settings || !vg_settings) return;

    RGFW_window_makeCurrentContext_OpenGL(settings);
    glViewport(0, 0, settings->w, settings->h);
    glClearColor(ui_clear_color[0], ui_clear_color[1], ui_clear_color[2], ui_clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg_settings, (float)settings->w, (float)settings->h, 1.0f);

    // Theme button
    drawButton(vg_settings, &btnTheme);

    // Voice Threshold Section
    char volText[64];
    sprintf(volText, "Voice Threshold: %.2f", g_config.voiceThreshold);
    drawLabel(vg_settings, volText, 20, 40);
    drawSlider(vg_settings, &sliderVolume);

    // Model Path Section
    drawLabel(vg_settings, "Default Model:", 20, 105);
    strncpy(txtModelPath.text, g_config.defaultModelPath, 511);
    drawTextDisplay(vg_settings, &txtModelPath);
    drawButton(vg_settings, &btnBrowse);

    // Viseme Training Section
    drawLabel(vg_settings, "Viseme Training:", 20, 185);

    const char* visNames[] = { "Silence", "CH", "OU", "AA" };
    float* confidences = VisemeGetConfidences();
    int visSlots[] = { VIS_SLOT_SILENCE, VIS_SLOT_CH, VIS_SLOT_OU, VIS_SLOT_AA };

    for (int i = 0; i < 4; i++) {
        drawLabel(vg_settings, visNames[i], 20, visBars[i].y + 16);
        visBars[i].value = confidences[visSlots[i]];
        if (visBars[i].value < 0) visBars[i].value = 0;
        if (visBars[i].value > 1) visBars[i].value = 1;
        drawProgressBar(vg_settings, &visBars[i]);
    }

    // Training buttons
    int trainSlots[] = { -1, VIS_SLOT_SILENCE, VIS_SLOT_CH, VIS_SLOT_OU, VIS_SLOT_AA };
    for (int i = 0; i < 5; i++) {
        int active = (currentTrainingSlot == trainSlots[i]);
        drawSmallButton(vg_settings, &visButtons[i], active);
    }
    drawSmallButton(vg_settings, &btnSaveViseme, 0);

    // Costume Hotkeys Section
    drawLabel(vg_settings, "Costume Hotkeys:", 20, 430);
    for (int i = 0; i < MAX_HOTKEYS; i++) {
        char label[8];
        sprintf(label, "C%d:", i + 1);
        drawLabel(vg_settings, label, hotkeyBtns[i].x - 45, hotkeyBtns[i].y + 22);
        hotkeyBtns[i].keyCode = g_config.hotkeys[i];
        drawHotkeyButton(vg_settings, &hotkeyBtns[i]);
    }

    nvgEndFrame(vg_settings);
    glFlush();
    RGFW_window_swapBuffers_OpenGL(settings);
}

#endif
