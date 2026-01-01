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
            // Random phase offset 0 to 2PI for independent direction
            part.phaseOffset = ((float)rand() / RAND_MAX) * 6.28318f;

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
                } else if (strstr(name, "[aa]")) {
                    part.type = LAYER_MOUTH_AA;
                } else if (strstr(name, "[ou]")) {
                    part.type = LAYER_MOUTH_OU;
                } else if (strstr(name, "[ch]")) {
                    part.type = LAYER_MOUTH_CH;
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

static float Lerp(float a, float b, float t) {
    return a + t * (b - a);
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

    // Viseme logic
    if (avatar->isTalking) {
        avatar->currentViseme = VisemeGetBest();
    } else {
        avatar->currentViseme = -1;
    }

    // Sway and Bounce Logic
    avatar->swayTime += deltaTime;
    
    // Clamp deltaTime to prevent physics jitters if window is moved or lags
    float physicsDt = deltaTime;
    if (physicsDt > 0.033f) physicsDt = 0.033f; 

    if (avatar->isTalking) {
        // Sway intensity based on volume
        float targetIntensity = volume * 3.0f * (3.14159f / 180.0f);
        
        // Attack (Fast rise)
        if (targetIntensity > avatar->swayIntensity) {
             avatar->swayIntensity = Lerp(avatar->swayIntensity, targetIntensity, 0.2f);
        } else {
             // Decay (Slow fall)
             avatar->swayIntensity = Lerp(avatar->swayIntensity, targetIntensity, 0.05f);
        }
    } else {
        // Decay to rest
        avatar->swayIntensity = Lerp(avatar->swayIntensity, 0.0f, 0.05f);
    }

    // Velocity-based Spring Bounce
    float targetBounce = (avatar->isTalking) ? (volume * 25.0f) : 0.0f;
    
    // Physics constants - Higher stiffness for faster bob
    const float stiffness = 300.0f;
    const float damping = 45.0f;

    // Spring calculation: a = (k * displacement) - (d * velocity)
    float displacement = targetBounce - avatar->bounceY;
    float springForce = displacement * stiffness;
    float dampingForce = avatar->bounceVelocity * damping;
    float acceleration = springForce - dampingForce;

    avatar->bounceVelocity += acceleration * physicsDt;
    avatar->bounceY += avatar->bounceVelocity * physicsDt;

    // Costume switching via hotkey
    if (hotkeyPressed > 0 && hotkeyPressed <= 9) {
        avatar->currentCostume = hotkeyPressed;
    }
}

static int HasLayerType(Avatar* avatar, LayerType type) {
    for (int i = 0; i < avatar->layerCount; i++) {
        if (avatar->layers[i].type == type && 
            (avatar->layers[i].costumeId == 0 || avatar->layers[i].costumeId == avatar->currentCostume)) {
            return 1;
        }
    }
    return 0;
}

static void DrawAvatar(void* vgCtx, Avatar* avatar, float windowWidth, float windowHeight) {
    NVGcontext* vg = (NVGcontext*)vgCtx;
    if (!avatar->isLoaded) return;

    // Determine which mouth layer to show
    LayerType mouthToShow = LAYER_MOUTH_CLOSED;
    if (avatar->isTalking) {
        mouthToShow = LAYER_MOUTH_OPEN;
        if (avatar->currentViseme == VIS_SLOT_AA && HasLayerType(avatar, LAYER_MOUTH_AA)) mouthToShow = LAYER_MOUTH_AA;
        else if (avatar->currentViseme == VIS_SLOT_OU && HasLayerType(avatar, LAYER_MOUTH_OU)) mouthToShow = LAYER_MOUTH_OU;
        else if (avatar->currentViseme == VIS_SLOT_CH && HasLayerType(avatar, LAYER_MOUTH_CH)) mouthToShow = LAYER_MOUTH_CH;
    }

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
        
        // Mouth logic
        if (part->type == LAYER_MOUTH_OPEN || part->type == LAYER_MOUTH_CLOSED || 
            part->type == LAYER_MOUTH_AA || part->type == LAYER_MOUTH_OU || part->type == LAYER_MOUTH_CH) {
            if (part->type != mouthToShow) continue;
        }

        if (part->costumeId > 0 && part->costumeId != avatar->currentCostume) continue;

        // Calculate position with scale
        float dx = offsetX + part->x * scale;
        float dy = offsetY + part->y * scale;
        float dw = part->width * scale;
        float dh = part->height * scale;
        
        // Apply bounce (subtract to move up)
        dy -= avatar->bounceY * scale;

        nvgSave(vg);

        // Apply Rotation if pivot exists
        if (part->hasPivot) {
            float px = dx + part->pivotOffsetX * scale;
            float py = dy + part->pivotOffsetY * scale;
            
            // Calculate individual layer rotation
            float layerAngle = sinf(avatar->swayTime * 15.0f + part->phaseOffset) * avatar->swayIntensity;

            nvgTranslate(vg, px, py);
            nvgRotate(vg, layerAngle);
            nvgTranslate(vg, -px, -py);
        }

        // Create image pattern
        NVGpaint imgPaint = nvgImagePattern(vg, dx, dy, dw, dh, 0, part->nvgImage, part->opacity);

        nvgBeginPath(vg);
        nvgRect(vg, dx, dy, dw, dh);
        nvgFillPaint(vg, imgPaint);
        nvgFill(vg);
        
        nvgRestore(vg);
    }
}

#endif
