#ifndef ORA_LOADER_H
#define ORA_LOADER_H

#include "avatar.h"
#include "attribute.h"
#include "miniz.h"
#include "nanovgXC/nanovg.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <sys/stat.h>
static long GetFileModTime(const char* path) {
    struct _stat st;
    if (_stat(path, &st) == 0) return (long)st.st_mtime;
    return 0;
}
#else
#include <sys/stat.h>
static long GetFileModTime(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) return (long)st.st_mtime;
    return 0;
}
#endif

static void ResolvePivots(Avatar* avatar, AvatarPart* pivotLayer) {
    char* targetName = pivotLayer->name + 6; // Skip "pivot_"
    for (int i = 0; i < avatar->layerCount; i++) {
        if (strcmp(avatar->layers[i].name, targetName) == 0) {
            avatar->layers[i].pivotOffsetX = (float)(pivotLayer->x - avatar->layers[i].x);
            avatar->layers[i].pivotOffsetY = (float)(pivotLayer->y - avatar->layers[i].y);
            avatar->layers[i].hasPivot = 1;
            return;
        }
    }
}

static void UnloadAvatar(void* vgCtx, Avatar* avatar) {
    NVGcontext* vg = (NVGcontext*)vgCtx;
    if (!avatar->isLoaded) return;
    for (int i = 0; i < avatar->layerCount; i++) {
        if (avatar->layers[i].nvgImage != 0) {
            nvgDeleteImage(vg, avatar->layers[i].nvgImage);
        }
    }
    memset(avatar, 0, sizeof(Avatar));
}

static void LoadAvatarFromOra(void* vgCtx, const char* filepath, Avatar* avatar) {
    NVGcontext* vg = (NVGcontext*)vgCtx;
    UnloadAvatar(vg, avatar);

    mz_zip_archive zip = {0};
    if (!mz_zip_reader_init_file(&zip, filepath, 0)) {
        printf("[Error] Failed to open ORA file: %s\n", filepath);
        return;
    }

    size_t xmlSize;
    char* xmlData = (char*)mz_zip_reader_extract_file_to_heap(&zip, "stack.xml", &xmlSize, 0);
    if (!xmlData) {
        printf("[Error] stack.xml missing in ORA file.\n");
        mz_zip_reader_end(&zip);
        return;
    }

    // Parse image dimensions
    char* imageTag = strstr(xmlData, "<image");
    if (imageTag) {
        avatar->width = GetIntAttribute(imageTag, "w");
        avatar->height = GetIntAttribute(imageTag, "h");
    }
    if (avatar->width == 0) { avatar->width = 500; avatar->height = 500; }

    // Temp storage for pivot layers
    AvatarPart pivotLayers[50];
    int pivotCount = 0;

    char* cursor = xmlData;
    while ((cursor = strstr(cursor, "<layer")) != NULL) {
        char* tagEnd = strchr(cursor, '>');
        if (!tagEnd) break;

        char currentTag[1024];
        int tagLen = (int)(tagEnd - cursor);
        if (tagLen > 1023) tagLen = 1023;
        strncpy(currentTag, cursor, tagLen);
        currentTag[tagLen] = '\0';

        // Check visibility
        char visibility[64];
        GetStringAttribute(currentTag, "visibility", visibility, 64);
        if (strcmp(visibility, "hidden") == 0) {
            cursor = tagEnd;
            continue;
        }

        float opacity = GetFloatAttribute(currentTag, "opacity", 1.0f);
        int x = GetIntAttribute(currentTag, "x");
        int y = GetIntAttribute(currentTag, "y");
        char src[128], name[128];
        GetStringAttribute(currentTag, "src", src, 128);
        GetStringAttribute(currentTag, "name", name, 128);

        size_t pngSize;
        void* pngData = mz_zip_reader_extract_file_to_heap(&zip, src, &pngSize, 0);

        if (pngData && avatar->layerCount < 100) {
            // Create NanoVG image from PNG data
            int nvgImg = nvgCreateImageMem(vg, 0, (unsigned char*)pngData, (int)pngSize);
            mz_free(pngData);

            if (nvgImg == 0) {
                cursor = tagEnd;
                continue;
            }

            AvatarPart part = {0};
            part.nvgImage = nvgImg;
            part.x = x;
            part.y = y;
            nvgImageSize(vg, nvgImg, &part.width, &part.height);
            part.opacity = opacity;
            part.active = 1;
            part.hasPivot = 0;

            ToLowerStr(name);
            strncpy(part.name, name, 63);
            part.name[63] = '\0';

            // Costume ID from name pattern: c[1-9]_*
            if (name[0] == 'c' && isdigit((unsigned char)name[1]) && name[2] == '_') {
                part.costumeId = name[1] - '0';
            } else {
                part.costumeId = 0;
            }

            part.type = LAYER_NORMAL;

            // Check for pivot layer
            if (strncmp(name, "pivot_", 6) == 0) {
                if (pivotCount < 50) pivotLayers[pivotCount++] = part;
                nvgDeleteImage(vg, nvgImg); // Don't keep pivot layer images
            } else {
                // Categorize layer type
                if (strstr(name, "blink") || (strstr(name, "eye") && strstr(name, "clos"))) {
                    part.type = LAYER_EYE_CLOSED;
                } else if (strstr(name, "eye")) {
                    part.type = LAYER_EYE_OPEN;
                } else if (strstr(name, "mouth") && (strstr(name, "open") || strstr(name, "talk"))) {
                    part.type = LAYER_MOUTH_OPEN;
                } else if (strstr(name, "mouth")) {
                    part.type = LAYER_MOUTH_CLOSED;
                }
                avatar->layers[avatar->layerCount++] = part;
            }
        }

        cursor = tagEnd;
    }

    // Resolve pivot offsets
    for (int i = 0; i < pivotCount; i++) {
        ResolvePivots(avatar, &pivotLayers[i]);
    }

    mz_free(xmlData);
    mz_zip_reader_end(&zip);

    avatar->isLoaded = 1;
    avatar->currentCostume = 1;
    avatar->blinkTimer = 0.0f;
    avatar->nextBlinkTime = 2.0f + (float)(rand() % 3);
    avatar->isBlinking = 0;
    avatar->isTalking = 0;

    printf("Avatar loaded: %dx%d with %d layers.\n", avatar->width, avatar->height, avatar->layerCount);

    strncpy(avatar->filePath, filepath, 1023);
    avatar->filePath[1023] = '\0';
    avatar->lastModTime = GetFileModTime(filepath);
}

static void UpdateAvatar(Avatar* avatar, float deltaTime, float volume, float threshold, int hotkeyPressed) {
    if (!avatar->isLoaded) return;

    // Blink logic
    avatar->blinkTimer += deltaTime;
    if (avatar->isBlinking) {
        // Blink duration ~150ms
        if (avatar->blinkTimer >= 0.15f) {
            avatar->isBlinking = 0;
            avatar->blinkTimer = 0.0f;
            avatar->nextBlinkTime = 2.0f + (float)(rand() % 4);
        }
    } else {
        if (avatar->blinkTimer >= avatar->nextBlinkTime) {
            avatar->isBlinking = 1;
            avatar->blinkTimer = 0.0f;
        }
    }

    // Talking based on volume threshold
    avatar->isTalking = (volume > threshold) ? 1 : 0;

    // Costume switching via hotkey
    if (hotkeyPressed > 0 && hotkeyPressed <= 9) {
        avatar->currentCostume = hotkeyPressed;
    }
}

static void DrawAvatar(void* vgCtx, Avatar* avatar, float windowWidth, float windowHeight) {
    NVGcontext* vg = (NVGcontext*)vgCtx;
    if (!avatar->isLoaded) return;

    // Calculate scale to fit avatar in window
    float scaleX = windowWidth / (float)avatar->width;
    float scaleY = windowHeight / (float)avatar->height;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    // Center the avatar
    float offsetX = (windowWidth - avatar->width * scale) / 2.0f;
    float offsetY = (windowHeight - avatar->height * scale) / 2.0f;

    // Draw layers in reverse order (back to front)
    for (int i = avatar->layerCount - 1; i >= 0; i--) {
        AvatarPart* part = &avatar->layers[i];

        // Skip based on state
        if (part->type == LAYER_EYE_CLOSED && !avatar->isBlinking) continue;
        if (part->type == LAYER_EYE_OPEN && avatar->isBlinking) continue;
        if (part->type == LAYER_MOUTH_OPEN && !avatar->isTalking) continue;
        if (part->type == LAYER_MOUTH_CLOSED && avatar->isTalking) continue;
        if (part->costumeId > 0 && part->costumeId != avatar->currentCostume) continue;

        // Calculate position with scale
        float dx = offsetX + part->x * scale;
        float dy = offsetY + part->y * scale;
        float dw = part->width * scale;
        float dh = part->height * scale;

        // Create image pattern
        NVGpaint imgPaint = nvgImagePattern(vg, dx, dy, dw, dh, 0, part->nvgImage, part->opacity);

        nvgBeginPath(vg);
        nvgRect(vg, dx, dy, dw, dh);
        nvgFillPaint(vg, imgPaint);
        nvgFill(vg);
    }
}

#endif
