#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"    
#include "backend.h"
#include "avatar.h"
#include "config.h"
#include <string.h>

void DrawLayer(AvatarPart* p, float bounceY, float swayAngle, int currentCostume, bool isTalking, bool isBlinking) {
    if (!p->active) return;
    if (p->costumeId != 0 && p->costumeId != currentCostume) return;

    if ((p->type == LAYER_EYE_OPEN     &&  isBlinking)||
        (p->type == LAYER_EYE_CLOSED   && !isBlinking)||
        (p->type == LAYER_MOUTH_OPEN   && !isTalking) ||
        (p->type == LAYER_MOUTH_CLOSED &&  isTalking) ){
        return;
    }

    if (p->hasPivot) {
        Rectangle source = { 0, 0, (float)p->texture.width, (float)p->texture.height };
        Rectangle dest = { 
            (float)p->x + p->pivotOffset.x, 
            (float)p->y + p->pivotOffset.y - bounceY, 
            (float)p->texture.width, 
            (float)p->texture.height 
        };
        Vector2 origin = p->pivotOffset;
        DrawTexturePro(p->texture, source, dest, origin, swayAngle, Fade(WHITE, p->opacity));
    } 
    else {
        DrawTexture(p->texture, p->x, (int)(p->y - bounceY), Fade(WHITE, p->opacity));
    }
}

int main(int argc, char **argv)
{
    SetTraceLogLevel(LOG_WARNING);
    
    const char* appDir = GetApplicationDirectory();
    char configPath[1024];
    strcpy(configPath, TextFormat("%s%s", appDir, "config.ini"));

    AppConfig config = {0};
    if (!FileExists(configPath)) {
        SaveDefaultConfig(configPath);
    }
    LoadConfig(configPath, &config);

    SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_ALWAYS_RUN );
    InitWindow(500, 500, "PNGTube ORA");
    SetWindowIconID(GetWindowHandle(), 101);
    SetTargetFPS(60);
    InitBackend();

    Avatar myAvatar = {0};

    if (argc > 1 && IsFileExtension(argv[1], ".ora")) {
        LoadAvatarFromOra(argv[1], &myAvatar);
    } else if (config.defaultModelPath[0] != '\0') {
        const char* defaultModelPath = TextFormat("%s%s", appDir, config.defaultModelPath);
        if (FileExists(defaultModelPath)) {
            LoadAvatarFromOra(defaultModelPath, &myAvatar);
        }
    }

    if (myAvatar.width > 0) SetWindowSize(myAvatar.width, myAvatar.height);
    
    while (!WindowShouldClose())
    {
        int key = GetConfiguredHotkey(config.hotkeys, MAX_HOTKEYS);
        if (key > 0) myAvatar.currentCostume = key;

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0 && IsFileExtension(droppedFiles.paths[0], ".ora")) {
                LoadAvatarFromOra(droppedFiles.paths[0], &myAvatar);
                if (myAvatar.width > 0) SetWindowSize(myAvatar.width, myAvatar.height);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        float vol = GetMicrophoneVolume();
        bool isTalking = vol > 0.15f;
        float dt = GetFrameTime();

        myAvatar.nextBlinkTime -= dt;
        if (myAvatar.nextBlinkTime <= 0) {
            myAvatar.isBlinking = true;
            myAvatar.blinkTimer = 0.15f;
            myAvatar.nextBlinkTime = 2.0f + ((float)GetRandomValue(0,400)/100.0f);
        }
        if (myAvatar.isBlinking) {
            myAvatar.blinkTimer -= dt;
            if (myAvatar.blinkTimer <= 0) myAvatar.isBlinking = false;
        }

        static float currentBounce = 0;
        static float currentSway = 0;

        if (isTalking) {
            currentBounce = Lerp(currentBounce, vol * 30.0f, 0.2f);
            float targetSway = sinf(GetTime() * 15.0f) * (vol * 10.0f); 
            currentSway = Lerp(currentSway, targetSway, 0.1f);
        } else {
            currentBounce = Lerp(currentBounce, 0, 0.1f);
            currentSway = Lerp(currentSway, 0, 0.1f);
        }

        BeginDrawing();
            ClearBackground(BLANK);

            BeginBlendMode(BLEND_ALPHA); 
            rlDisableDepthMask();
            rlDisableDepthTest();

            if (myAvatar.isLoaded) {
                for (int i = myAvatar.layerCount - 1; i >= 0; i--) {
                    DrawLayer(&myAvatar.layers[i], 
                              currentBounce, 
                              currentSway, 
                              myAvatar.currentCostume, 
                              isTalking, 
                              myAvatar.isBlinking);
                }
            } else {
                DrawRectangle(0,0, 500, 500, BLACK);
                DrawText("Drop ORA File", 140, 240, 30, WHITE);
            }
        EndDrawing();
    }

    UnloadAvatar(&myAvatar);
    CloseBackend();
    CloseWindow();
    return 0;
}