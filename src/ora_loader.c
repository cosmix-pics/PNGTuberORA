#include "avatar.h"
#include "miniz.h"
#include "attribute.h"

void UnloadAvatar(Avatar* avatar) {
    if (!avatar->isLoaded) return;
    for(int i=0; i<avatar->layerCount; i++) {
        UnloadTexture(avatar->layers[i].texture);
    }
    memset(avatar, 0, sizeof(Avatar));
}

void ResolvePivots(Avatar* avatar, AvatarPart* pivotLayer) {
    char* targetName = pivotLayer->name + 6; 
    for (int i=0; i<avatar->layerCount; i++) {
        if (strcmp(avatar->layers[i].name, targetName) == 0) {
            avatar->layers[i].pivotOffset.x = (float)(pivotLayer->x - avatar->layers[i].x);
            avatar->layers[i].pivotOffset.y = (float)(pivotLayer->y - avatar->layers[i].y);
            avatar->layers[i].hasPivot = true;
            return;
        }
    }
}

void LoadAvatarFromOra(const char* filepath, Avatar* avatar) {
    UnloadAvatar(avatar);

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

    char* imageTag = strstr(xmlData, "<image");
    if (imageTag) {
        avatar->width = GetIntAttribute(imageTag, "w");
        avatar->height = GetIntAttribute(imageTag, "h");
    }
    if (avatar->width == 0) { avatar->width = 500; avatar->height = 500; }

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
            Image img = LoadImageFromMemory(".png", pngData, (int)pngSize);
            Texture2D tex = LoadTextureFromImage(img);
            UnloadImage(img);
            mz_free(pngData);

            AvatarPart part = {0};
            part.texture = tex;
            part.x = x; 
            part.y = y;
            part.opacity = opacity;
            part.active = true;
            part.hasPivot = false; 

            ToLowerStr(name);
            strcpy(part.name, name);

            if (name[0] == 'c' && isdigit(name[1]) && name[2] == '_') {
                part.costumeId = name[1] - '0';
            } else {
                part.costumeId = 0;
            }
            part.type = LAYER_NORMAL; 
            if (strncmp(name, "pivot_", 6) == 0) {
                if (pivotCount < 50) pivotLayers[pivotCount++] = part;
                UnloadTexture(tex);
            }
            else {
                if (strstr(name, "blink") || (strstr(name, "eye") && strstr(name, "clos"))) {
                    part.type = LAYER_EYE_CLOSED;
                } 
                else if (strstr(name, "eye")) {
                    part.type = LAYER_EYE_OPEN;
                }
                else if (strstr(name, "mouth") && (strstr(name, "open") || strstr(name, "talk"))) {
                    part.type = LAYER_MOUTH_OPEN;
                }
                else if (strstr(name, "mouth")) {
                    part.type = LAYER_MOUTH_CLOSED;
                }
                avatar->layers[avatar->layerCount++] = part;
            }
        }
        
        cursor = tagEnd;
    }
    for (int i=0; i<pivotCount; i++) {
        ResolvePivots(avatar, &pivotLayers[i]);
    }
    mz_free(xmlData);
    mz_zip_reader_end(&zip);
    
    avatar->isLoaded = true;
    avatar->currentCostume = 1;
    printf("Avatar loaded successfully: %dx%d with %d layers.\n", avatar->width, avatar->height, avatar->layerCount);
}