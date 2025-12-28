#include "settings.h"
#include "backend.h"
#include "tigr.h"
#include "viseme_trainer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define CLAY_IMPLEMENTATION
#include "clay.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
    #include <strings.h>
    #define _strnicmp strncasecmp
#endif

typedef struct {
    AppConfig* config;
    char configPath[1024];
    char visemePath[1024];
    int waitingForKeyIdx;
} SettingsData;

static bool settingsOpen = false;
static void* g_clayMemoryPtr = NULL;
static bool g_clayInitialized = false;

void Tigr_ClayErrorHandler(Clay_ErrorData errorData) {
    (void)errorData;
}

Clay_Dimensions Tigr_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    (void)config; (void)userData;
    static char buf[1024];
    int len = text.length < 1023 ? text.length : 1023;
    memcpy(buf, text.chars, len);
    buf[len] = '\0';
    return (Clay_Dimensions) { (float)tigrTextWidth(tfont, buf), (float)tigrTextHeight(tfont, buf) };
}

void Clay_Tigr_Render(Tigr* screen, Clay_RenderCommandArray renderCommands) {
    for (int i = 0; i < renderCommands.length; i++) {
        Clay_RenderCommand* cmd = Clay_RenderCommandArray_Get(&renderCommands, i);
        switch (cmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData* data = &cmd->renderData.rectangle;
                tigrFillRect(screen, (int)cmd->boundingBox.x - 1, (int)cmd->boundingBox.y - 1, (int)cmd->boundingBox.width + 2, (int)cmd->boundingBox.height + 2, 
                    tigrRGBA((unsigned char)data->backgroundColor.r, (unsigned char)data->backgroundColor.g, (unsigned char)data->backgroundColor.b, (unsigned char)data->backgroundColor.a));
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData* data = &cmd->renderData.text;
                static char buf[1024];
                int len = data->stringContents.length < 1023 ? data->stringContents.length : 1023;
                memcpy(buf, data->stringContents.chars, len);
                buf[len] = '\0';
                tigrPrint(screen, tfont, (int)cmd->boundingBox.x, (int)cmd->boundingBox.y, 
                    tigrRGBA((unsigned char)data->textColor.r, (unsigned char)data->textColor.g, (unsigned char)data->textColor.b, (unsigned char)data->textColor.a), 
                    "%s", buf);
                break;
            }
            default: break;
        }
    }
}

Clay_String Clay_String_From_Char_Buffer(const char* buf) {
    return (Clay_String) { .length = (int)strlen(buf), .chars = buf, .isStaticallyAllocated = false };
}

Clay_String Clay_CopyString(const char* buf) {
    return Clay__WriteStringToCharBuffer(&Clay_GetCurrentContext()->dynamicStringData, Clay_String_From_Char_Buffer(buf));
}

void SettingsThread(void* arg) {
    SettingsData* data = (SettingsData*)arg;
    Tigr* screen = tigrWindow(100, 100, "Settings", TIGR_2X | TIGR_AUTO);
    if (!screen) {
        free(data);
        settingsOpen = false;
        return;
    }

#ifdef _WIN32
    HWND hwnd = (HWND)screen->handle;
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOZORDER);
#endif

    SetWindowIconID(screen->handle, 101);

    if (!g_clayInitialized) {
        uint32_t clayMemorySize = Clay_MinMemorySize();
        g_clayMemoryPtr = malloc(clayMemorySize);
        Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(clayMemorySize, g_clayMemoryPtr);
        Clay_Initialize(clayMemory, (Clay_Dimensions) { 400, 500 }, (Clay_ErrorHandler) { Tigr_ClayErrorHandler, NULL });
        Clay_SetMeasureTextFunction(Tigr_MeasureText, NULL);
        g_clayInitialized = true;
    }
    
    int currentTrainingSlot = -1;

    while (!tigrClosed(screen)) {
        int mx, my, mb;
        tigrMouse(screen, &mx, &my, &mb);
        static int lastMb = 0;
        bool click = (mb & 1) && !(lastMb & 1);

        Clay_SetPointerState((Clay_Vector2) { (float)mx, (float)my }, mb & 1);
        Clay_SetLayoutDimensions((Clay_Dimensions) { (float)screen->w, (float)screen->h });
        
        Clay_BeginLayout();
        CLAY({
            .id = CLAY_ID("MainContainer"),
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .padding = { 16, 16, 16, 16 }, .childGap = 16, .sizing = { .width = CLAY_SIZING_FIT() } },
            .backgroundColor = { 40, 40, 45, 255 }
        }) {
           
            // Voice Threshold Section
            CLAY({ .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 8 } }) {
                char volText[64];
                sprintf(volText, "Voice Threshold: %.2f", data->config->voiceThreshold);
                CLAY_TEXT(Clay_CopyString(volText), CLAY_TEXT_CONFIG({ .textColor = { 200, 200, 200, 255 } }));
                
                CLAY({
                    .id = CLAY_ID("SliderBG"),
                    .layout = { .sizing = { .width = CLAY_SIZING_FIXED(200), .height = CLAY_SIZING_FIXED(12) } },
                    .backgroundColor = { 60, 60, 65, 255 }
                }) {
                    Clay_ElementData sliderData = Clay_GetElementData(CLAY_ID("SliderBG"));
                    int sliderX = 0;
                    if (sliderData.found) {
                        sliderX = (int)(data->config->voiceThreshold * sliderData.boundingBox.width);
                    }
                    
                    CLAY({
                        .id = CLAY_ID("SliderHandle"),
                        .layout = { .sizing = { .width = CLAY_SIZING_FIXED(10), .height = CLAY_SIZING_FIXED(20) } },
                        .floating = { .offset = { (float)sliderX - 5, -4 }, .attachTo = CLAY_ATTACH_TO_PARENT },
                        .backgroundColor = { 100, 150, 255, 255 }
                    }) {}
                }
            }

            // Handle slider input
            if (mb & 1) {
                Clay_ElementData sliderData = Clay_GetElementData(CLAY_ID("SliderBG"));
                if (sliderData.found && mx >= sliderData.boundingBox.x && mx <= sliderData.boundingBox.x + sliderData.boundingBox.width &&
                    my >= sliderData.boundingBox.y - 10 && my <= sliderData.boundingBox.y + sliderData.boundingBox.height + 10) {
                    float newVal = (mx - sliderData.boundingBox.x) / sliderData.boundingBox.width;
                    if (newVal < 0) newVal = 0;
                    if (newVal > 1) newVal = 1;
                    data->config->voiceThreshold = newVal;
                    SaveConfig(data->configPath, data->config);
                }
            }

            // Model Path Section
            CLAY({ .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 8 } }) {
                CLAY_TEXT(CLAY_STRING("Default Model:"), CLAY_TEXT_CONFIG({ .textColor = { 200, 200, 200, 255 } }));
                CLAY({ .layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT, .childGap = 8 } }) {
                    CLAY({
                        .layout = { .sizing = { .width = CLAY_SIZING_FIXED(200), .height = CLAY_SIZING_FIXED(20) }, .padding = { 4, 4, 4, 4 } },
                        .backgroundColor = { 60, 60, 65, 255 }
                    }) {
                        CLAY_TEXT(Clay_CopyString(data->config->defaultModelPath), CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 } }));
                    }
                    CLAY({
                        .id = CLAY_ID("BrowseBtn"),
                        .layout = { .padding = { 8, 8, 4, 4 } },
                        .backgroundColor = Clay_PointerOver(CLAY_ID("BrowseBtn")) ? (Clay_Color){ 100, 100, 105, 255 } : (Clay_Color){ 80, 80, 85, 255 }
                    }) {
                        CLAY_TEXT(CLAY_STRING("Browse..."), CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 } }));
                    }
                }
            }

            if (click && Clay_PointerOver(CLAY_ID("BrowseBtn"))) {
                char* selectedFile = OpenFileDialog(screen->handle, "Select ORA File", "ORA Files\0*.ora\0All Files\0*.*\0");
                if (selectedFile) {
                    strncpy(data->config->defaultModelPath, selectedFile, sizeof(data->config->defaultModelPath) - 1);
                    SaveConfig(data->configPath, data->config);
                    free(selectedFile);
                }
            }

            // Viseme Training Section
            CLAY({ .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 8 } }) {
                CLAY_TEXT(CLAY_STRING("Viseme Training:"), CLAY_TEXT_CONFIG({ .textColor = { 200, 200, 200, 255 } }));
                
                float* confidences = VisemeGetConfidences();
                const char* visNames[] = { "None", "Silence", "CH", "OU", "AA" };
                int visSlots[] = { -1, VIS_SLOT_SILENCE, VIS_SLOT_CH, VIS_SLOT_OU, VIS_SLOT_AA };

                for (int i = 1; i <= 4; i++) {
                    CLAY({ .layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT, .childGap = 8, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } } }) {
                        CLAY({ .layout = { .sizing = { .width = CLAY_SIZING_FIXED(60) } } }) {
                            CLAY_TEXT(Clay_CopyString(visNames[i]), CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 } }));
                        }
                        
                        CLAY({
                            .layout = { .sizing = { .width = CLAY_SIZING_FIXED(100), .height = CLAY_SIZING_FIXED(10) } },
                            .backgroundColor = { 60, 60, 65, 255 }
                        }) {
                            float conf = confidences[visSlots[i]];
                            if (conf < 0) conf = 0;
                            if (conf > 1) conf = 1;
                            CLAY({
                                .layout = { .sizing = { .width = CLAY_SIZING_PERCENT(conf), .height = CLAY_SIZING_GROW() } },
                                .backgroundColor = { 100, 255, 100, 255 }
                            }) {}
                        }
                    }
                }

                CLAY({ .layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT, .childGap = 4, .padding = { 0, 0, 8, 0 } } }) {
                    for (int i = 0; i <= 4; i++) {
                        Clay_ElementId btnId = Clay_GetElementIdWithIndex(CLAY_STRING("TrainBtn"), i);
                        bool active = currentTrainingSlot == visSlots[i];
                        CLAY({
                            .id = btnId,
                            .layout = { .padding = { 6, 6, 4, 4 } },
                            .backgroundColor = active ? (Clay_Color){ 100, 150, 255, 255 } : (Clay_PointerOver(btnId) ? (Clay_Color){ 80, 80, 85, 255 } : (Clay_Color){ 60, 60, 65, 255 })
                        }) {
                            CLAY_TEXT(Clay_CopyString(visNames[i]), CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 }, .fontSize = 12 }));
                        }

                        if (click && Clay_PointerOver(btnId)) {
                            currentTrainingSlot = visSlots[i];
                            VisemeSetTraining(currentTrainingSlot);
                        }
                    }
                }

                CLAY({
                    .id = CLAY_ID("SaveVisBtn"),
                    .layout = { .padding = { 8, 8, 4, 4 }, .sizing = { .width = CLAY_SIZING_GROW() } },
                    .backgroundColor = Clay_PointerOver(CLAY_ID("SaveVisBtn")) ? (Clay_Color){ 100, 100, 105, 255 } : (Clay_Color){ 80, 80, 85, 255 }
                }) {
                    CLAY_TEXT(CLAY_STRING("Save Viseme Data"), CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 }, .fontSize = 12, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
                }

                if (click && Clay_PointerOver(CLAY_ID("SaveVisBtn"))) {
                    VisemeSave(data->visemePath);
                }
            }

            // Costume Hotkeys
            CLAY({ .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 8, .sizing = { .width = CLAY_SIZING_GROW() } } }) {
                CLAY_TEXT(CLAY_STRING("Costume Hotkeys:"), CLAY_TEXT_CONFIG({ .textColor = { 200, 200, 200, 255 } }));
                for (int i = 0; i < MAX_HOTKEYS; i++) {
                    CLAY({ .layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT, .childGap = 8, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } } }) {
                        char cLabel[16];
                        sprintf(cLabel, "C%d:", i + 1);
                        CLAY_TEXT(Clay_CopyString(cLabel), CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 } }));
                        
                        char keyName[16];
                        int vk = data->config->hotkeys[i];
                        if (vk >= 0x70 && vk <= 0x7B) sprintf(keyName, "F%d", vk - 0x70 + 1);
                        else if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) sprintf(keyName, "%c", (char)vk);
                        else sprintf(keyName, "0x%02X", vk);
                        if (data->waitingForKeyIdx == i) strcpy(keyName, "...");

                        Clay_ElementId btnId = Clay_GetElementIdWithIndex(CLAY_STRING("HotkeyBtn"), i);
                        CLAY({
                            .id = btnId,
                            .layout = { .sizing = { .width = CLAY_SIZING_FIXED(60) }, .padding = { 8, 8, 4, 4 } },
                            .backgroundColor = Clay_PointerOver(btnId) ? (Clay_Color){ 80, 80, 85, 255 } : (Clay_Color){ 60, 60, 65, 255 }
                        }) {
                            CLAY_TEXT(Clay_CopyString(keyName), CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 } }));
                        }

                        if (click && Clay_PointerOver(btnId)) {
                            data->waitingForKeyIdx = i;
                        }
                    }
                }
            }
        }
        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        Clay_ElementData containerData = Clay_GetElementData(CLAY_ID("MainContainer"));
        if (containerData.found) {
            int targetW = (int)containerData.boundingBox.width;
            int targetH = (int)containerData.boundingBox.height;
            if (targetW != screen->w || targetH != screen->h) {
#ifdef _WIN32
                HWND hwnd = (HWND)screen->handle;
                RECT rcW, rcC;
                GetWindowRect(hwnd, &rcW);
                GetClientRect(hwnd, &rcC);
                int borderW = (rcW.right - rcW.left) - (rcC.right - rcC.left);
                int borderH = (rcW.bottom - rcW.top) - (rcC.bottom - rcC.top);
                SetWindowPos(hwnd, NULL, 0, 0, targetW * 2 + borderW, targetH * 2 + borderH, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
#else
                extern void tigrResize(Tigr* bmp, int w, int h);
                tigrResize(screen, targetW, targetH);
#endif
            }
        }

        if (data->waitingForKeyIdx != -1) {
            for (int k = 1; k < 255; k++) {
                if (tigrKeyDown(screen, k)) {
                    int mapped = k;
                    if (k >= 'a' && k <= 'z') mapped = k - 'a' + 'A';
                    if (k >= TK_F1 && k <= TK_F12) mapped = 0x70 + (k - TK_F1);
                    data->config->hotkeys[data->waitingForKeyIdx] = mapped;
                    data->waitingForKeyIdx = -1;
                    SaveConfig(data->configPath, data->config);
                    break;
                }
            }
        }

        tigrClear(screen, tigrRGB(40, 40, 45));
        Clay_Tigr_Render(screen, renderCommands);
        lastMb = mb;
        tigrUpdate(screen);
    }
    tigrFree(screen);
    free(data);
    settingsOpen = false;
}

#ifndef _WIN32
static void* PosixSettingsThread(void* arg) {
    SettingsThread(arg);
    return NULL;
}
#endif

void OpenSettingsWindow(AppConfig* config, const char* configPath, const char* visemePath) {
    if (settingsOpen) return;
    settingsOpen = true;
    SettingsData* data = malloc(sizeof(SettingsData));
    data->config = config;
    strcpy(data->configPath, configPath);
    strcpy(data->visemePath, visemePath);
    data->waitingForKeyIdx = -1;
#ifdef _WIN32
    uintptr_t _beginthread(void( * )( void * ), unsigned, void * );
    _beginthread(SettingsThread, 0, data);
#else
    pthread_t thread;
    pthread_create(&thread, NULL, PosixSettingsThread, data);
    pthread_detach(thread);
#endif
}