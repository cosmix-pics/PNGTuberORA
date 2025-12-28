#ifndef CONFIG_H
#define CONFIG_H

#define MAX_HOTKEYS 9

typedef struct {
    char defaultModelPath[512];
    int hotkeys[MAX_HOTKEYS];
    float voiceThreshold;
} AppConfig;

void LoadConfig(const char* filename, AppConfig* config);
void SaveDefaultConfig(const char* filename);
void SaveConfig(const char* filename, const AppConfig* config);

#endif
