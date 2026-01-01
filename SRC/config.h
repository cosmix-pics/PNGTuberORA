#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_HOTKEYS 9

typedef struct {
    char defaultModelPath[512];
    int hotkeys[MAX_HOTKEYS];
    float voiceThreshold;
    int theme; // 0=Trans, 1=Dark, 2=White
} AppConfig;

static int ParseKeyName(const char* name) {
    if (!name || !*name) return 0;
    if (strlen(name) == 1) {
        char c = toupper(name[0]);
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) {
            return (int)c;
        }
    }
    if (toupper(name[0]) == 'F') {
        int fNum = atoi(name + 1);
        if (fNum >= 1 && fNum <= 12) {
            return 0x70 + (fNum - 1);
        }
    }
    return 0;
}

static const char* GetVKName(int vk) {
    static char buf[4];
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        buf[0] = (char)vk;
        buf[1] = '\0';
        return buf;
    }
    if (vk >= 0x70 && vk <= 0x7B) {
        sprintf(buf, "F%d", vk - 0x70 + 1);
        return buf;
    }
    return "0";
}

static void TrimString(char* str) {
    char* p = str;
    int l = (int)strlen(p);
    if (l == 0) return;
    while (l > 0 && isspace((unsigned char)p[l - 1])) p[--l] = 0;
    while (*p && isspace((unsigned char)*p)) ++p, --l;
    if (p != str) memmove(str, p, l + 1);
}

static void LoadConfig(const char* filename, AppConfig* config) {
    config->defaultModelPath[0] = '\0';
    for (int i = 0; i < MAX_HOTKEYS; i++) {
        config->hotkeys[i] = '1' + i;
    }
    config->voiceThreshold = 0.15f;

    FILE* f = fopen(filename, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';
        TrimString(line);
        if (strlen(line) == 0) continue;

        char* eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = line;
        char* val = eq + 1;
        TrimString(key);
        TrimString(val);

        if (strcmp(key, "default_model") == 0) {
            strncpy(config->defaultModelPath, val, sizeof(config->defaultModelPath) - 1);
        } else if (strcmp(key, "voice_threshold") == 0) {
            config->voiceThreshold = (float)atof(val);
        } else if (strcmp(key, "theme") == 0) {
            config->theme = atoi(val);
        } else if (strncmp(key, "costume_", 8) == 0) {
            int idx = atoi(key + 8) - 1;
            if (idx >= 0 && idx < MAX_HOTKEYS) {
                int vk = ParseKeyName(val);
                if (vk != 0) config->hotkeys[idx] = vk;
            }
        }
    }
    fclose(f);
}

static void SaveConfig(const char* filename, const AppConfig* config) {
    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "# PNGTuber ORA Configuration\n");
    fprintf(f, "default_model = %s\n", config->defaultModelPath);
    fprintf(f, "voice_threshold = %.3f\n", config->voiceThreshold);
    fprintf(f, "theme = %d\n", config->theme);
    fprintf(f, "\n# Hotkeys for costumes 1-9 (Format: A-Z, 0-9, F1-F12)\n");
    for (int i = 0; i < MAX_HOTKEYS; i++) {
        fprintf(f, "costume_%d = %s\n", i + 1, GetVKName(config->hotkeys[i]));
    }
    fclose(f);
}

static void SaveDefaultConfig(const char* filename) {
    AppConfig config = {0};
    strcpy(config.defaultModelPath, "");
    for (int i = 0; i < MAX_HOTKEYS; i++) config.hotkeys[i] = '1' + i;
    config.voiceThreshold = 0.15f;
    config.theme = 0;
    SaveConfig(filename, &config);
}

#endif
