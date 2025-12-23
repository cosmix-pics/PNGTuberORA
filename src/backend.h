#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>

void InitBackend(void);
void CloseBackend(void);
float GetMicrophoneVolume(void);
int GetConfiguredHotkey(const int* keyBindings, int count);
void SetWindowIconID(void* hwnd, int iconId);

#endif