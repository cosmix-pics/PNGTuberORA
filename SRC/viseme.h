#ifndef VISEME_H
#define VISEME_H

#define TINYVIS_IMPLEMENTATION
#include "tinyvis.h"
#include <stdio.h>

#define VIS_SLOT_SILENCE 1
#define VIS_SLOT_CH      2
#define VIS_SLOT_OU      3
#define VIS_SLOT_AA      4

static tvis_ctx g_visemeCtx;
static int g_visemeTrainingSlot = -1;
static int g_visemeInitialized = 0;

static void VisemeInit(void) {
    if (!tvis_init(&g_visemeCtx)) {
        printf("[Error] Failed to init viseme system\n");
        return;
    }
    g_visemeInitialized = 1;
}

static void VisemeShutdown(void) {
    if (g_visemeInitialized) {
        tvis_free(&g_visemeCtx);
        g_visemeInitialized = 0;
    }
}

void VisemeProcess(const float* pcm, int frameCount) {
    if (!g_visemeInitialized) return;
    if (frameCount < TVIS_FFT_SIZE) return;

    tvis_analyze(&g_visemeCtx, pcm);

    if (g_visemeTrainingSlot > 0) {
        tvis_train(&g_visemeCtx, g_visemeTrainingSlot);
    }
}

static void VisemeSetTraining(int slot) {
    g_visemeTrainingSlot = slot;
}

static int VisemeSave(const char* filename) {
    if (tvis_save(&g_visemeCtx, filename)) {
        printf("Saved visemes to %s\n", filename);
        return 1;
    } else {
        printf("[Error] Failed to save visemes to %s\n", filename);
        return 0;
    }
}

static int VisemeLoad(const char* filename) {
    return tvis_load(&g_visemeCtx, filename);
}

static int VisemeGetBest(void) {
    int best = -1;
    float max_conf = 0.0f;

    for (int i = 1; i <= 4; i++) {
        if (g_visemeCtx.confidences[i] > max_conf) {
            max_conf = g_visemeCtx.confidences[i];
            best = i;
        }
    }

    if (max_conf < 0.3f) return -1;
    return best;
}

static float* VisemeGetConfidences(void) {
    return g_visemeCtx.confidences;
}

static void VisemeClearSlot(int slot) {
    tvis_clear_slot(&g_visemeCtx, slot);
}

#endif
