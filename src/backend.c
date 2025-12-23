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

    static Display* x11Display = NULL;
    static bool x11Initialized = false;
    static char prevKeyState[32] = {0};

    // Convert Windows VK code / ASCII to X11 KeySym
    static KeySym VKToKeySym(int vk) {
        // VK Letters and numbers are just ACSII
        if (vk >= 'A' && vk <= 'Z') {
            // Lowercase, because of course X11 has separate keycodes for both
            return XK_a + (vk - 'A');
        }
        if (vk >= '0' && vk <= '9') {
            return XK_0 + (vk - '0');
        }
        // Numpad 0-9 (Windows VK: 0x60-0x69)
        if (vk >= 0x60 && vk <= 0x69) {
            return XK_KP_0 + (vk - 0x60);
        }
        // Function keys F1-F24 (Windows VK: 0x70-0x87)
        // X11 goes up to F35, but Windows stops at F24
        if (vk >= 0x70 && vk <= 0x87) {
            return XK_F1 + (vk - 0x70);
        }
        return NoSymbol;
    }

    int GetConfiguredHotkey(const int* keyBindings, int count) {
        // Lazy initialization of X11 display
        if (!x11Initialized) {
            x11Display = XOpenDisplay(NULL);
            x11Initialized = true;
        }
        if (!x11Display) return 0;

        char keyState[32];
        XQueryKeymap(x11Display, keyState);

        for (int i = 0; i < count; i++) {
            if (keyBindings[i] <= 0) continue;

            KeySym keysym = VKToKeySym(keyBindings[i]);
            if (keysym == NoSymbol) continue;

            KeyCode keycode = XKeysymToKeycode(x11Display, keysym);
            if (keycode == 0) continue;

            // Key presses are stored in a bitmap
            int byteIndex = keycode / 8;
            int bitIndex = keycode % 8;

            bool isPressed = (keyState[byteIndex] & (1 << bitIndex)) != 0;
            bool wasPressed = (prevKeyState[byteIndex] & (1 << bitIndex)) != 0;

            // X only seems to track the current state, so save it for the next loop
            if (isPressed && !wasPressed) {
                memcpy(prevKeyState, keyState, sizeof(prevKeyState));
                return i + 1;
            }
        }

        memcpy(prevKeyState, keyState, sizeof(prevKeyState));
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
#if defined(__linux__)
    if (x11Display) {
        XCloseDisplay(x11Display);
        x11Display = NULL;
    }
#endif
}
float GetMicrophoneVolume(void) {
    float vol = currentVolume * 5.0f; 
    if (vol > 1.0f) vol = 1.0f;
    return vol;
}
