#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static void ToLowerStr(char *str) {
    for(; *str; ++str) *str = (char)tolower((unsigned char)*str);
}

static int GetIntAttribute(const char* tagString, const char* attrName) {
    char search[64];
    sprintf(search, "%s=", attrName);

    const char* p = tagString;
    while ((p = strstr(p, search)) != NULL) {
        char prev = (p == tagString) ? ' ' : *(p - 1);
        if (isspace((unsigned char)prev)) {
            p += strlen(search);
            if (*p == '"' || *p == '\'') {
                p++;
                return atoi(p);
            }
        }
        p++;
    }
    return 0;
}

static float GetFloatAttribute(const char* tagString, const char* attrName, float defaultValue) {
    char search[64];
    sprintf(search, "%s=", attrName);
    const char* p = tagString;
    while ((p = strstr(p, search)) != NULL) {
        char prev = (p == tagString) ? ' ' : *(p - 1);
        if (isspace((unsigned char)prev)) {
            p += strlen(search);
            if (*p == '"' || *p == '\'') {
                p++;
                return (float)atof(p);
            }
        }
        p++;
    }
    return defaultValue;
}

static void GetStringAttribute(const char* tagString, const char* attrName, char* outBuffer, int maxLen) {
    char search[64];
    sprintf(search, "%s=", attrName);
    const char* p = tagString;
    while ((p = strstr(p, search)) != NULL) {
        char prev = (p == tagString) ? ' ' : *(p - 1);

        if (isspace((unsigned char)prev)) {
            p += strlen(search);
            char quote = *p;
            if (quote == '"' || quote == '\'') {
                p++;
                const char* end = strchr(p, quote);
                if (!end) { outBuffer[0] = 0; return; }
                int len = (int)(end - p);
                if (len > maxLen - 1) len = maxLen - 1;
                strncpy(outBuffer, p, len);
                outBuffer[len] = 0;
                return;
            }
        }
        p++;
    }
    outBuffer[0] = 0;
}

#endif
