#include "viseme_trainer.h"
#define TINYVIS_IMPLEMENTATION
#include "tinyvis.h"
#include <stdio.h>

static tvis_ctx viseme_ctx;
static int training_slot = -1;

void VisemeInit(void) {
    if (!tvis_init(&viseme_ctx)) {
        printf("Failed to init tinyvis\n");
    }
}

void VisemeShutdown(void) {
    tvis_free(&viseme_ctx);
}

void VisemeProcess(const float* pcm, int frameCount) {
    if (frameCount < TVIS_FFT_SIZE) return;

    tvis_analyze(&viseme_ctx, pcm);

    if (training_slot > 0) {
        tvis_train(&viseme_ctx, training_slot);
    }
}

void VisemeSetTraining(int slot) {
    training_slot = slot;
}

void VisemeSave(const char* filename) {
    if (tvis_save(&viseme_ctx, filename)) {
        printf("Saved visemes to %s\n", filename);
    } else {
        printf("Failed to save visemes to %s\n", filename);
    }
}

int VisemeLoad(const char* filename) {
    return tvis_load(&viseme_ctx, filename);
}

int VisemeGetBest(void) {
    int best = -1;
    float max_conf = 0.0f;
    
    for (int i = 1; i <= 4; i++) {
        if (viseme_ctx.confidences[i] > max_conf) {
            max_conf = viseme_ctx.confidences[i];
            best = i;
        }
    }
    
    if (max_conf < 0.3f) return -1;
    
    return best;
}

float* VisemeGetConfidences(void) {
    return viseme_ctx.confidences;
}
