#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"    
#include "backend.h"
#include "settings.h"
#include "avatar.h"
#include "config.h"
#include "viseme_trainer.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

bool IsPathAbsolute(const char* path) {
    if (!path || path[0] == '\0') return false;
    if (strlen(path) >= 2 && isalpha(path[0]) && path[1] == ':') return true;
    if (path[0] == '/' || path[0] == '\\') return true;
    return false;
}

void DrawLayer(AvatarPart* p, float bounceY, float swayAngle, int currentCostume, bool isTalking, bool isBlinking, AvatarPart* activeMouth) {
    if (!p->active) return;
    if (p->costumeId != 0 && p->costumeId != currentCostume) return;

    if ((p->type == LAYER_EYE_OPEN     &&  isBlinking)||
        (p->type == LAYER_EYE_CLOSED   && !isBlinking)||
        (p->type == LAYER_MOUTH_CLOSED &&  isTalking) ){
        return;
    }

    if (p->type == LAYER_MOUTH_OPEN) {
        if (!isTalking) return;
        if (activeMouth && p != activeMouth) return;
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
    
    char visemePath[1024];
    strcpy(visemePath, TextFormat("%s%s", appDir, "visime.dat"));

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
    VisemeInit();

    bool isTrainingMode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-visime") == 0) {
            isTrainingMode = true;
        }
    }

    if (VisemeLoad(visemePath)) {
        printf("Loaded visemes from %s\n", visemePath);
    }

    Avatar myAvatar = {0};
    char lastLoadedModel[512] = "";

    if (argc > 1 && IsFileExtension(argv[1], ".ora")) {
        LoadAvatarFromOra(argv[1], &myAvatar);
        strncpy(lastLoadedModel, argv[1], sizeof(lastLoadedModel) - 1);
    } else if (config.defaultModelPath[0] != '\0') {
        char fullModelPath[1024];
        if (IsPathAbsolute(config.defaultModelPath)) {
            strcpy(fullModelPath, config.defaultModelPath);
        } else {
            sprintf(fullModelPath, "%s%s", appDir, config.defaultModelPath);
        }
        
        if (FileExists(fullModelPath)) {
            LoadAvatarFromOra(fullModelPath, &myAvatar);
            strncpy(lastLoadedModel, config.defaultModelPath, sizeof(lastLoadedModel) - 1);
        }
    }

    if (myAvatar.width > 0) SetWindowSize(myAvatar.width, myAvatar.height);
    
    float reloadTimer = 0.0f;

    while (!WindowShouldClose())
    {
        if (strcmp(lastLoadedModel, config.defaultModelPath) != 0) {
            char fullModelPath[1024];
            if (IsPathAbsolute(config.defaultModelPath)) {
                strcpy(fullModelPath, config.defaultModelPath);
            } else {
                sprintf(fullModelPath, "%s%s", appDir, config.defaultModelPath);
            }

            if (FileExists(fullModelPath)) {
                Avatar tempAvatar = {0};
                LoadAvatarFromOra(fullModelPath, &tempAvatar);
                if (tempAvatar.isLoaded) {
                    UnloadAvatar(&myAvatar);
                    myAvatar = tempAvatar;
                    if (myAvatar.width > 0) SetWindowSize(myAvatar.width, myAvatar.height);
                    strncpy(lastLoadedModel, config.defaultModelPath, sizeof(lastLoadedModel) - 1);
                }
            } else {
                // If the file doesn't exist, we still mark it as "seen" to avoid repeated attempts
                strncpy(lastLoadedModel, config.defaultModelPath, sizeof(lastLoadedModel) - 1);
            }
        }

        int key = GetConfiguredHotkey(config.hotkeys, MAX_HOTKEYS);
        if (key > 0) myAvatar.currentCostume = key;

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            int menuResult = ShowContextMenu(GetWindowHandle());
            if (menuResult == 1) { // Settings
                OpenSettingsWindow(&config, configPath, visemePath);
            } else if (menuResult == 2) { // Quit
                break;
            } else if (menuResult == 3) { // Toggle Border
                if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
                    ClearWindowState(FLAG_WINDOW_UNDECORATED);
                } else {
                    SetWindowState(FLAG_WINDOW_UNDECORATED);
                }
            }
        }

        if (isTrainingMode) {
            if (IsKeyDown(KEY_FOUR)) VisemeSetTraining(VIS_SLOT_AA);
            else if (IsKeyDown(KEY_THREE)) VisemeSetTraining(VIS_SLOT_OU);
            else if (IsKeyDown(KEY_TWO)) VisemeSetTraining(VIS_SLOT_CH);
            else if (IsKeyDown(KEY_ONE)) VisemeSetTraining(VIS_SLOT_SILENCE);
            else VisemeSetTraining(-1);

            if (IsKeyPressed(KEY_ZERO)) VisemeSave(visemePath);
        }

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0 && IsFileExtension(droppedFiles.paths[0], ".ora")) {
                LoadAvatarFromOra(droppedFiles.paths[0], &myAvatar);
                if (myAvatar.width > 0) SetWindowSize(myAvatar.width, myAvatar.height);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        float vol = GetMicrophoneVolume();
        bool isTalking = vol > config.voiceThreshold;
        float dt = GetFrameTime();

        reloadTimer += dt;
        if (reloadTimer >= 1.0f) {
            reloadTimer = 0.0f;
            if (myAvatar.isLoaded && FileExists(myAvatar.filePath)) {
                long currentModTime = GetFileModTime(myAvatar.filePath);
                if (currentModTime > myAvatar.lastModTime) {
                    Avatar tempAvatar = {0};
                    LoadAvatarFromOra(myAvatar.filePath, &tempAvatar);
                    
                    if (tempAvatar.isLoaded) {
                        UnloadAvatar(&myAvatar);
                        myAvatar = tempAvatar;
                        if (myAvatar.width > 0) SetWindowSize(myAvatar.width, myAvatar.height);
                    } else {
                        UnloadAvatar(&tempAvatar);
                    }
                }
            }
        }

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

        AvatarPart* activeMouth = NULL;
        if (isTalking && myAvatar.isLoaded) {
            int bestSlot = VisemeGetBest();
            AvatarPart* defaultMouth = NULL;
            
            for (int i = 0; i < myAvatar.layerCount; i++) {
                AvatarPart* p = &myAvatar.layers[i];
                if (p->type == LAYER_MOUTH_OPEN && p->active && (p->costumeId == 0 || p->costumeId == myAvatar.currentCostume)) {
                    bool isSpecific = (strstr(p->name, "[aa]") || strstr(p->name, "[ou]") || strstr(p->name, "[ch]"));
                    if (!isSpecific) defaultMouth = p;
                    
                    if (bestSlot == VIS_SLOT_AA && strstr(p->name, "[aa]")) activeMouth = p;
                    else if (bestSlot == VIS_SLOT_OU && strstr(p->name, "[ou]")) activeMouth = p;
                    else if (bestSlot == VIS_SLOT_CH && strstr(p->name, "[ch]")) activeMouth = p;
                }
            }
            if (!activeMouth) activeMouth = defaultMouth;
        }

        BeginDrawing();
            ClearBackground(BLANK);

            if (myAvatar.isLoaded) {
                rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
                BeginBlendMode(BLEND_CUSTOM_SEPARATE);
                for (int i = myAvatar.layerCount - 1; i >= 0; i--) {
                    DrawLayer(&myAvatar.layers[i], 
                              currentBounce, 
                              currentSway, 
                              myAvatar.currentCostume, 
                              isTalking, 
                              myAvatar.isBlinking,
                              activeMouth);
                }
                EndBlendMode();
            } else {
                DrawRectangle(0,0, 500, 500, BLACK);
                DrawText("Drop ORA File", 140, 240, 30, WHITE);
            }
            
            if (isTrainingMode) {
                DrawText("VIS TRAINING MODE", 10, 10, 20, RED);
                DrawText("1: Silence 2: CH 3: OU 4: AA 0: Save", 10, 30, 10, WHITE);
                float* conf = VisemeGetConfidences();
                for(int i=1; i<=4; i++) {
                    DrawRectangle(10 + (i*30), 50, 20, (int)(conf[i]*100), GREEN);
                    DrawText(TextFormat("%d", i), 10 + (i*30), 155, 10, WHITE);
                }
            }
        EndDrawing();
    }

    UnloadAvatar(&myAvatar);
    VisemeShutdown();
    CloseBackend();
    CloseWindow();
    return 0;
}