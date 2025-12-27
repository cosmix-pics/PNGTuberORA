#ifndef AVATAR_H
#define AVATAR_H

#include "raylib.h"

typedef enum {
    LAYER_NORMAL = 0,
    LAYER_EYE_OPEN,
    LAYER_EYE_CLOSED,
    LAYER_MOUTH_OPEN,
    LAYER_MOUTH_CLOSED
} LayerType;

typedef struct {
    Texture2D texture;
    int x; 
    int y;
    
    LayerType type;
    int costumeId;
    Vector2 pivotOffset;  
    bool hasPivot;        
    char name[64];        

    float opacity;
    bool active;
} AvatarPart;

typedef struct {
    int width;
    int height;
    AvatarPart layers[100]; 
    int layerCount;
    int currentCostume; 
    float swayAngle;    
    float blinkTimer;
    float nextBlinkTime;
    bool isBlinking;
    bool isLoaded;

    char filePath[1024];
    long lastModTime;
} Avatar;

void LoadAvatarFromOra(const char* filepath, Avatar* avatar);
void UnloadAvatar(Avatar* avatar);

#endif