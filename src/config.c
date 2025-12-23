#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

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

static void Trim(char* str) {
    char* p = str;
    int l = strlen(p);
    while(isspace(p[l - 1])) p[--l] = 0;
    while(*p && isspace(*p)) ++p, --l;
    memmove(str, p, l + 1);
}

void SaveDefaultConfig(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "# PNGTuber ORA Configuration\n");
    fprintf(f, "default_model = \n");
    fprintf(f, "\n# Hotkeys for costumes 1-9 (Format: A-Z, 0-9, F1-F12)\n");
    for (int i = 0; i < MAX_HOTKEYS; i++) {
        fprintf(f, "costume_%d = %d\n", i + 1, i + 1);
    }
    fclose(f);
}

void LoadConfig(const char* filename, AppConfig* config) {
    config->defaultModelPath[0] = '\0';
    for (int i = 0; i < MAX_HOTKEYS; i++) {
        config->hotkeys[i] = '1' + i;
    }

    FILE* f = fopen(filename, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';
        Trim(line);
        if (strlen(line) == 0) continue;

        char* eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = line;
        char* val = eq + 1;
        Trim(key);
        Trim(val);

        if (strcmp(key, "default_model") == 0) {
            strncpy(config->defaultModelPath, val, sizeof(config->defaultModelPath) - 1);
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
