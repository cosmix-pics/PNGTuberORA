#ifndef AUDIO_H
#define AUDIO_H

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdio.h>
#include <math.h>

// Forward declaration for viseme processing
void VisemeProcess(const float* pcm, int frameCount);

static ma_device g_audioDevice;
static volatile float g_currentVolume = 0.0f;
static int g_audioInitialized = 0;

static void audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* pInputFloat = (const float*)pInput;
    if (pInputFloat == NULL) return;

    // Feed audio to viseme processor
    VisemeProcess(pInputFloat, (int)frameCount);

    // Calculate RMS volume
    float sum = 0.0f;
    for (ma_uint32 i = 0; i < frameCount; i++) {
        float sample = pInputFloat[i];
        sum += sample * sample;
    }
    float rms = sqrtf(sum / frameCount);

    // Smooth volume changes
    if (rms > g_currentVolume) {
        g_currentVolume = rms;
    } else {
        g_currentVolume *= 0.95f;
    }

    (void)pDevice;
    (void)pOutput;
}

static int InitAudio(void) {
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate = 44100;
    config.periodSizeInFrames = 512;
    config.dataCallback = audio_data_callback;

    if (ma_device_init(NULL, &config, &g_audioDevice) != MA_SUCCESS) {
        printf("[Error] Failed to initialize audio capture device.\n");
        return 0;
    }

    if (ma_device_start(&g_audioDevice) != MA_SUCCESS) {
        printf("[Error] Failed to start audio capture.\n");
        ma_device_uninit(&g_audioDevice);
        return 0;
    }

    g_audioInitialized = 1;
    printf("Audio capture initialized.\n");
    return 1;
}

static void CloseAudio(void) {
    if (g_audioInitialized) {
        ma_device_uninit(&g_audioDevice);
        g_audioInitialized = 0;
    }
}

static float GetMicrophoneVolume(void) {
    float vol = g_currentVolume * 5.0f;
    if (vol > 1.0f) vol = 1.0f;
    return vol;
}

#endif
