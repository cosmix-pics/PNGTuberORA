#ifndef CONFIG_H
#define CONFIG_H

#define MAX_HOTKEYS 9

typedef struct {
    char defaultModelPath[512];
    int hotkeys[MAX_HOTKEYS];
} AppConfig;

void LoadConfig(const char* filename, AppConfig* config);
void SaveDefaultConfig(const char* filename);

#endif
