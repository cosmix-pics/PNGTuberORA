#ifndef DIALOG_H
#define DIALOG_H

#include "shared.h"
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEP "\\"
#else
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEP "/"
#endif

typedef struct {
    char name[260];
    int is_dir;
    float x, y, w, h;
} FileEntry;

static char currentPath[512] = ".";
static FileEntry files[100];
static int fileCount = 0;

void scanDirectory(const char* path) {
    fileCount = 0;

#ifdef _WIN32
    char searchPath[512];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(searchPath, &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") == 0) continue;

            int is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            int is_ora = 0;
            if (!is_dir) {
                char* ext = strrchr(fd.cFileName, '.');
                if (ext && strcmp(ext, ".ora") == 0) is_ora = 1;
            }

            if (is_dir || is_ora) {
                if (fileCount < 100) {
                    strncpy(files[fileCount].name, fd.cFileName, 259);
                    files[fileCount].is_dir = is_dir;
                    fileCount++;
                }
            }
        } while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0) continue;

            char fullPath[512];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

            struct stat st;
            int is_dir = 0;
            if (stat(fullPath, &st) == 0) {
                is_dir = S_ISDIR(st.st_mode);
            }

            int is_ora = 0;
            if (!is_dir) {
                char* ext = strrchr(entry->d_name, '.');
                if (ext && strcmp(ext, ".ora") == 0) is_ora = 1;
            }

            if (is_dir || is_ora) {
                if (fileCount < 100) {
                    strncpy(files[fileCount].name, entry->d_name, 259);
                    files[fileCount].is_dir = is_dir;
                    fileCount++;
                }
            }
        }
        closedir(dir);
    }
#endif
}

void dialog_init(void) {
    dialog = RGFW_createWindow("Select Avatar", 500, 600, 500, 600, RGFW_windowOpenGL | RGFW_windowNoResize);
    if (dialog) {
        backend_set_window_icon(dialog);
        RGFW_window_makeCurrentContext_OpenGL(dialog);
        gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL);
        vg_dialog = nvglCreate(NVGL_DEBUG);
        nvgCreateFontMem(vg_dialog, "sans", font_data, font_data_len, 0);
        RGFW_window_swapInterval_OpenGL(dialog, 0);
        
        scanDirectory(currentPath);
    }
}

void dialog_handle_event(RGFW_event* event) {
    if (event->type == RGFW_quit) {
        RGFW_window_makeCurrentContext_OpenGL(dialog);
        if (vg_dialog) nvglDelete(vg_dialog);
        RGFW_window_close(dialog);
        dialog = NULL; vg_dialog = NULL;
        return;
    }

    if (event->type == RGFW_mousePosChanged) {
        int hover = 0;
         for (int i = 0; i < fileCount; i++) {
            if (isPointInRect(event->mouse.x, event->mouse.y, files[i].x, files[i].y, files[i].w, files[i].h)) {
                hover = 1;
                break;
            }
        }
        if (hover) RGFW_window_setMouseStandard(dialog, RGFW_mousePointingHand);
        else RGFW_window_setMouseStandard(dialog, RGFW_mouseNormal);
    }

    if (event->type == RGFW_mouseButtonPressed && event->button.value == RGFW_mouseLeft) {
        i32 mx, my;
        RGFW_window_getMouse(dialog, &mx, &my);
        
        for (int i = 0; i < fileCount; i++) {
            if (isPointInRect(mx, my, files[i].x, files[i].y, files[i].w, files[i].h)) {
                if (files[i].is_dir) {
                    // Navigate
                    if (strcmp(files[i].name, "..") == 0) {
                        // Go up
                        char* lastSlash = strrchr(currentPath, '/');
                        if (!lastSlash) lastSlash = strrchr(currentPath, '\\');
                        
                        if (lastSlash) {
                             *lastSlash = '\0';
                        } else {
                            // If no slash, and we are not at ".", maybe we are at "subdir", so go back to "."
                            if (strcmp(currentPath, ".") != 0) strcpy(currentPath, ".");
                        }
                    } else {
                        // Go down
                        char newPath[512];
                        if (strcmp(currentPath, ".") == 0)
                            snprintf(newPath, sizeof(newPath), "%s", files[i].name);
                        else
                            snprintf(newPath, sizeof(newPath), "%s" PATH_SEP "%s", currentPath, files[i].name);
                        strcpy(currentPath, newPath);
                    }
                    scanDirectory(currentPath);
                } else {
                    // Select file
                    printf("Selected: %s\n", files[i].name);
                    // Close dialog
                    RGFW_window_makeCurrentContext_OpenGL(dialog);
                    if (vg_dialog) nvglDelete(vg_dialog);
                    RGFW_window_close(dialog);
                    dialog = NULL; vg_dialog = NULL;
                }
                break;
            }
        }
    }
}

void dialog_draw(void) {
    if (!dialog) return;
    RGFW_window_makeCurrentContext_OpenGL(dialog);
    glViewport(0, 0, dialog->w, dialog->h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    nvgBeginFrame(vg_dialog, (float)dialog->w, (float)dialog->h, 1.0f);
    
    // Path Label
    drawLabel(vg_dialog, currentPath, 20, 30);
    
    // Separator
    nvgBeginPath(vg_dialog);
    nvgMoveTo(vg_dialog, 20, 40);
    nvgLineTo(vg_dialog, 480, 40);
    nvgStrokeColor(vg_dialog, TRANS_WHITE);
    nvgStrokeWidth(vg_dialog, 1.0f);
    nvgStroke(vg_dialog);

    // Grid
    float startY = 60;
    float itemW = 140;
    float itemH = 60;
    float gap = 20;
    int cols = 3;

    for (int i = 0; i < fileCount; i++) {
        int col = i % cols;
        int row = i / cols;
        
        float x = 20 + col * (itemW + gap);
        float y = startY + row * (itemH + gap);
        
        // Store layout for hit testing
        files[i].x = x; files[i].y = y; files[i].w = itemW; files[i].h = itemH;
        
        // Draw Box
        nvgBeginPath(vg_dialog);
        nvgRoundedRect(vg_dialog, x, y, itemW, itemH, 5.0f);
        if (files[i].is_dir) nvgFillColor(vg_dialog, nvgRGBA(60, 60, 80, 255)); // Folder color
        else nvgFillColor(vg_dialog, nvgRGBA(80, 40, 60, 255)); // File color
        nvgFill(vg_dialog);
        
        nvgBeginPath(vg_dialog);
        nvgStrokeColor(vg_dialog, TRANS_WHITE);
        nvgStrokeWidth(vg_dialog, 1.0f);
        nvgRoundedRect(vg_dialog, x, y, itemW, itemH, 5.0f);
        nvgStroke(vg_dialog);
        
        // Text
        nvgFillColor(vg_dialog, UI_TEXT_COLOR);
        nvgFontSize(vg_dialog, 16.0f);
        nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg_dialog, x + itemW/2, y + itemH/2, files[i].name, NULL);
    }

    nvgEndFrame(vg_dialog);
    glFlush();
    RGFW_window_swapBuffers_OpenGL(dialog);
}

#endif