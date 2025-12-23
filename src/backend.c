#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "backend.h"
#include <stdio.h>
#include <math.h>

static ma_device device;
static volatile float currentVolume = 0.0f;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    const float* pInputFloat = (const float*)pInput;
    if (pInputFloat == NULL) return;

    float sum = 0.0f;
    for (ma_uint32 i = 0; i < frameCount; i++) {
        float sample = *pInputFloat++;
        sum += sample * sample;
    }
    float rms = sqrtf(sum / frameCount);
    if (rms > currentVolume) {
        currentVolume = rms;
    } else {
        currentVolume *= 0.95f;
    }
    (void)pOutput;
}

#ifdef _WIN32
    #include <windows.h>
    int GetConfiguredHotkey(const int* keyBindings, int count) {
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
    
    int GetConfiguredHotkey(const int* keyBindings, int count) {
        // TODO: Implement key mapping for Linux
        (void)keyBindings;
        (void)count;
        return 0;
    }
#else
    int GetConfiguredHotkey(const int* keyBindings, int count) { (void)keyBindings; (void)count; return 0; }
#endif

#ifdef _WIN32
void SetWindowIconID(void* hwnd, int iconId) {
    HWND hWindow = (HWND)hwnd;
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(iconId)); 
    if (hIcon) {
        SendMessage(hWindow, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
}
#else
void SetWindowIconID(void* hwnd, int iconId) {
    (void)hwnd;
    (void)iconId;
}
#endif

void InitBackend(void) {
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format   = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate       = 44100;
    config.dataCallback     = data_callback;

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        printf("Failed to initialize capture device.\n");
        return;
    }
    ma_device_start(&device);
}
void CloseBackend(void) {
    ma_device_uninit(&device);
}
float GetMicrophoneVolume(void) {
    float vol = currentVolume * 5.0f; 
    if (vol > 1.0f) vol = 1.0f;
    return vol;
}
