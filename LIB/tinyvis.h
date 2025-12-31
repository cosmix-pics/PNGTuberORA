#ifndef TINYVIS_H
#define TINYVIS_H

#include <stdint.h>

#define TVIS_FFT_SIZE 512
#define TVIS_SLOT_COUNT 10  // Slots 0-9 (1-9 for viseme)
#define TVIS_SMOOTHING 0.5f // 0.0 = instant, 0.9 = very slow transition

typedef struct {
    float bins[TVIS_FFT_SIZE / 2];
    int train_count;
} tvis_model_t;

typedef struct {
    tvis_model_t models[TVIS_SLOT_COUNT];
    float confidences[TVIS_SLOT_COUNT];
    
    float window_lut[TVIS_FFT_SIZE];
    float fft_real[TVIS_FFT_SIZE];
    float fft_imag[TVIS_FFT_SIZE];
    float magnitude[TVIS_FFT_SIZE / 2];
    
    float *sin_table;
    float *cos_table;
    int *bit_reverse_table;
} tvis_ctx;

int tvis_init(tvis_ctx* ctx);
void tvis_free(tvis_ctx* ctx);
void tvis_analyze(tvis_ctx* ctx, const float* pcm);
void tvis_train(tvis_ctx* ctx, int slot);
void tvis_clear_slot(tvis_ctx* ctx, int slot);
int tvis_save(tvis_ctx* ctx, const char* filename);
int tvis_load(tvis_ctx* ctx, const char* filename);

#endif
#ifdef TINYVIS_IMPLEMENTATION

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void tvis__gen_bit_reverse(int* table, int n) {
    int bits = 0;
    while ((1 << bits) < n) bits++;
    
    for (int i = 0; i < n; i++) {
        int r = 0;
        int val = i;
        for (int j = 0; j < bits; j++) {
            r = (r << 1) | (val & 1);
            val >>= 1;
        }
        table[i] = r;
    }
}

static void tvis__fft(tvis_ctx* ctx) {
    int n = TVIS_FFT_SIZE;
    for (int i = 0; i < n; i++) {
        int rev = ctx->bit_reverse_table[i];
        if (i < rev) {
            float tr = ctx->fft_real[i];
            float ti = ctx->fft_imag[i];
            ctx->fft_real[i] = ctx->fft_real[rev];
            ctx->fft_imag[i] = ctx->fft_imag[rev];
            ctx->fft_real[rev] = tr;
            ctx->fft_imag[rev] = ti;
        }
    }

    int lut_idx_step = 1;
    for (int len = 2; len <= n; len <<= 1) {
        int half_len = len >> 1;
        int step = (TVIS_FFT_SIZE / len); 
        
        for (int i = 0; i < n; i += len) {
            for (int j = 0; j < half_len; j++) {
                int k = j * step; 
                float c = ctx->cos_table[k];
                float s = ctx->sin_table[k]; 
                float u_r = ctx->fft_real[i + j];
                float u_i = ctx->fft_imag[i + j];
                float t_r = ctx->fft_real[i + j + half_len] * c - ctx->fft_imag[i + j + half_len] * s;
                float t_i = ctx->fft_real[i + j + half_len] * s + ctx->fft_imag[i + j + half_len] * c;

                ctx->fft_real[i + j] = u_r + t_r;
                ctx->fft_imag[i + j] = u_i + t_i;
                ctx->fft_real[i + j + half_len] = u_r - t_r;
                ctx->fft_imag[i + j + half_len] = u_i - t_i;
            }
        }
    }
}

int tvis_init(tvis_ctx* ctx) {
    memset(ctx, 0, sizeof(tvis_ctx));
    ctx->sin_table = (float*)malloc(sizeof(float) * (TVIS_FFT_SIZE / 2));
    ctx->cos_table = (float*)malloc(sizeof(float) * (TVIS_FFT_SIZE / 2));
    ctx->bit_reverse_table = (int*)malloc(sizeof(int) * TVIS_FFT_SIZE);

    if (!ctx->sin_table || !ctx->cos_table || !ctx->bit_reverse_table) return 0;

    for (int i = 0; i < TVIS_FFT_SIZE; i++) {
        ctx->window_lut[i] = 0.5f * (1.0f - cosf((2.0f * M_PI * i) / (TVIS_FFT_SIZE - 1)));
    }

    for (int i = 0; i < TVIS_FFT_SIZE / 2; i++) {
        ctx->cos_table[i] = cosf(-2.0f * M_PI * i / TVIS_FFT_SIZE);
        ctx->sin_table[i] = sinf(-2.0f * M_PI * i / TVIS_FFT_SIZE);
    }
    tvis__gen_bit_reverse(ctx->bit_reverse_table, TVIS_FFT_SIZE);

    return 1;
}

void tvis_free(tvis_ctx* ctx) {
    if (ctx->sin_table) free(ctx->sin_table);
    if (ctx->cos_table) free(ctx->cos_table);
    if (ctx->bit_reverse_table) free(ctx->bit_reverse_table);
}

void tvis_analyze(tvis_ctx* ctx, const float* pcm) {
    for (int i = 0; i < TVIS_FFT_SIZE; i++) {
        ctx->fft_real[i] = pcm[i] * ctx->window_lut[i];
        ctx->fft_imag[i] = 0.0f;
    }
    tvis__fft(ctx);
    float total_energy = 0.0f;
    for (int i = 0; i < TVIS_FFT_SIZE / 2; i++) {
        float r = ctx->fft_real[i];
        float im = ctx->fft_imag[i];
        float mag = sqrtf(r*r + im*im);
        ctx->magnitude[i] = mag;
        total_energy += mag * mag;
    }
    
    total_energy = sqrtf(total_energy);
    if (total_energy > 0.00001f) {
        for (int i = 0; i < TVIS_FFT_SIZE / 2; i++) {
            ctx->magnitude[i] /= total_energy;
        }
    } else {
        memset(ctx->magnitude, 0, sizeof(ctx->magnitude));
    }

    for (int s = 0; s < TVIS_SLOT_COUNT; s++) {
        if (ctx->models[s].train_count == 0) {
            ctx->confidences[s] = 0.0f;
            continue;
        }

        float dist_sq = 0.0f;
        for (int i = 0; i < TVIS_FFT_SIZE / 2; i++) {
            float d = ctx->magnitude[i] - ctx->models[s].bins[i];
            dist_sq += d * d;
        }
        
        float raw_conf = 1.0f - sqrtf(dist_sq); 
        if (raw_conf < 0.0f) raw_conf = 0.0f;
        ctx->confidences[s] = (ctx->confidences[s] * TVIS_SMOOTHING) + (raw_conf * (1.0f - TVIS_SMOOTHING));
    }
}

void tvis_train(tvis_ctx* ctx, int slot) {
    if (slot < 0 || slot >= TVIS_SLOT_COUNT) return;
    
    tvis_model_t* m = &ctx->models[slot];
    m->train_count++;
    
    float inv_n = 1.0f / (float)m->train_count;
    
    for (int i = 0; i < TVIS_FFT_SIZE / 2; i++) {
        m->bins[i] += (ctx->magnitude[i] - m->bins[i]) * inv_n;
    }
    
    float total = 0.0f;
    for(int i=0; i<TVIS_FFT_SIZE/2; i++) total += m->bins[i] * m->bins[i];
    total = sqrtf(total);
    if(total > 0.0001f) {
        for(int i=0; i<TVIS_FFT_SIZE/2; i++) m->bins[i] /= total;
    }
}

void tvis_clear_slot(tvis_ctx* ctx, int slot) {
    if (slot >= 0 && slot < TVIS_SLOT_COUNT) {
        memset(&ctx->models[slot], 0, sizeof(tvis_model_t));
    }
}

int tvis_save(tvis_ctx* ctx, const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) return 0;
    size_t w = fwrite(ctx->models, sizeof(tvis_model_t), TVIS_SLOT_COUNT, f);
    fclose(f);
    return (w == TVIS_SLOT_COUNT);
}

int tvis_load(tvis_ctx* ctx, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return 0;
    size_t r = fread(ctx->models, sizeof(tvis_model_t), TVIS_SLOT_COUNT, f);
    fclose(f);
    return (r == TVIS_SLOT_COUNT);
}
#endif