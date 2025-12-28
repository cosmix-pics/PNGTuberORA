#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>

#include "config.h"

void InitBackend(void);
void CloseBackend(void);
float GetMicrophoneVolume(void);
int GetConfiguredHotkey(const int* keyBindings, int count);
void SetWindowIconID(void* hwnd, int iconId);
int ShowContextMenu(void* windowHandle);
char* OpenFileDialog(void* windowHandle, const char* title, const char* filter);

#endif