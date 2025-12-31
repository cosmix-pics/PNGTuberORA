#ifndef AVATAR_H
#define AVATAR_H

typedef enum {
    LAYER_NORMAL = 0,
    LAYER_EYE_OPEN,
    LAYER_EYE_CLOSED,
    LAYER_MOUTH_OPEN,
    LAYER_MOUTH_CLOSED
} LayerType;

typedef struct {
    int nvgImage;           // NanoVG image handle
    int x, y;               // Position in avatar space
    int width, height;      // Layer dimensions
    LayerType type;
    int costumeId;          // 0 = all costumes, 1-9 = specific costume
    float pivotOffsetX;
    float pivotOffsetY;
    int hasPivot;
    char name[64];
    float opacity;
    int active;
} AvatarPart;

typedef struct {
    int width, height;      // Avatar canvas size
    AvatarPart layers[100];
    int layerCount;
    int currentCostume;
    float swayAngle;
    float blinkTimer;
    float nextBlinkTime;
    int isBlinking;
    int isTalking;
    int isLoaded;
    char filePath[1024];
    long lastModTime;
} Avatar;

// Function declarations - implementations in ora_loader.h (header-only, static inline)

#endif
