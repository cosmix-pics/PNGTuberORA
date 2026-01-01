#ifndef DIALOG_H
#define DIALOG_H

#include "shared.h"
#include "ora_loader.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEP "\\"
#else
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEP "/"
#endif

// Dialog window dimensions
#define DIALOG_WIDTH 560
#define DIALOG_HEIGHT 520

typedef struct {
    char name[260];
    int is_dir;
    float x, y, w, h;
    int hovered;
    int thumbnailImage;      // NanoVG image handle
    int thumbnailLoaded;     // 0=not loaded, 1=loaded, -1=failed
} FileEntry;

static FileEntry files[100];
static int fileCount = 0;
static Button btnDialogClose = { 420, 10, 120, 28, "Cancel", 0 };
static int dialog_scroll_offset = 0;

static int loadThumbnailFromOra(NVGcontext* vg, const char* oraPath) {
    if (!vg) return 0;

    mz_zip_archive zip = {0};
    if (!mz_zip_reader_init_file(&zip, oraPath, 0)) return 0;

    size_t thumbSize;
    void* thumbData = mz_zip_reader_extract_file_to_heap(&zip,
                      "Thumbnails/thumbnail.png", &thumbSize, 0);

    int nvgImg = 0;
    if (thumbData) {
        nvgImg = nvgCreateImageMem(vg, 0, (unsigned char*)thumbData, (int)thumbSize);
        mz_free(thumbData);
    }

    mz_zip_reader_end(&zip);
    return nvgImg;
}

static void cleanupThumbnails(NVGcontext* vg) {
    if (!vg) return;
    for (int i = 0; i < fileCount; i++) {
        if (files[i].thumbnailImage > 0) {
            nvgDeleteImage(vg, files[i].thumbnailImage);
            files[i].thumbnailImage = 0;
        }
        files[i].thumbnailLoaded = 0;
    }
}

static void scanDirectory(const char* path) {
    cleanupThumbnails(vg_dialog);

    fileCount = 0;
    dialog_scroll_offset = 0;

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
                if (ext && _stricmp(ext, ".ora") == 0) is_ora = 1;
            }

            if (is_dir || is_ora) {
                if (fileCount < 100) {
                    strncpy(files[fileCount].name, fd.cFileName, 259);
                    files[fileCount].is_dir = is_dir;
                    files[fileCount].hovered = 0;
                    files[fileCount].thumbnailImage = 0;
                    files[fileCount].thumbnailLoaded = 0;
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
                if (ext && strcasecmp(ext, ".ora") == 0) is_ora = 1;
            }

            if (is_dir || is_ora) {
                if (fileCount < 100) {
                    strncpy(files[fileCount].name, entry->d_name, 259);
                    files[fileCount].is_dir = is_dir;
                    files[fileCount].hovered = 0;
                    files[fileCount].thumbnailImage = 0;
                    files[fileCount].thumbnailLoaded = 0;
                    fileCount++;
                }
            }
        }
        closedir(dir);
    }
#endif
}

void dialog_set_path(const char* path) {
    strncpy(g_dialogPath, path, 511);
    g_dialogPath[511] = '\0';
}

void dialog_init(void) {
    DeactivateCurrentGLContext();

#ifdef _WIN32
    char absPath[512];
    if (_fullpath(absPath, g_dialogPath, sizeof(absPath)) != NULL) {
        strncpy(g_dialogPath, absPath, sizeof(g_dialogPath) - 1);
        g_dialogPath[sizeof(g_dialogPath) - 1] = '\0';
    }
#else
    char absPath[512];
    if (realpath(g_dialogPath, absPath) != NULL) {
        strncpy(g_dialogPath, absPath, sizeof(g_dialogPath) - 1);
        g_dialogPath[sizeof(g_dialogPath) - 1] = '\0';
    }
#endif

    dialog = RGFW_createWindow("Select Avatar", 150, 150, DIALOG_WIDTH, DIALOG_HEIGHT,
                                RGFW_windowOpenGL);
    if (!dialog) {
        printf("[Error] Failed to create dialog window\n");
        return;
    }

    backend_set_window_icon(dialog);
    RGFW_window_makeCurrentContext_OpenGL(dialog);
    gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL);

    vg_dialog = nvglCreate(NVGL_DEBUG);
    if (!vg_dialog) {
        printf("[Error] Failed to create NanoVG context for dialog\n");
        RGFW_window_close(dialog);
        dialog = NULL;
        return;
    }

    nvgCreateFontMem(vg_dialog, "sans", font_data, font_data_len, 0);
    RGFW_window_swapInterval_OpenGL(dialog, 1);

    scanDirectory(g_dialogPath);
}

void dialog_close(void) {
    if (dialog) {
        RGFW_window_makeCurrentContext_OpenGL(dialog);
        cleanupThumbnails(vg_dialog);
        if (vg_dialog) {
            nvglDelete(vg_dialog);
            vg_dialog = NULL;
        }
        RGFW_window_close(dialog);
        dialog = NULL;
    }
}

void dialog_handle_event(RGFW_event* event) {
    if (!dialog || !vg_dialog) return;

    if (event->type == RGFW_quit) {
        dialog_close();
        return;
    }

    i32 mx, my;
    RGFW_window_getMouse(dialog, &mx, &my);

    float startY = 60;
    float itemW = 160;
    float itemH = 140;
    float gap = 12;
    
    int cols = (dialog->w - 40) / (int)(itemW + gap);
    if (cols < 1) cols = 1;

    btnDialogClose.x = dialog->w - btnDialogClose.w - 20;

    for (int i = 0; i < fileCount; i++) {
        int col = i % cols;
        int row = i / cols;

        files[i].x = 20 + col * (itemW + gap);
        files[i].y = startY + row * (itemH + gap) - dialog_scroll_offset;
        files[i].w = itemW;
        files[i].h = itemH;
    }

    if (event->type == RGFW_mousePosChanged) {
        btnDialogClose.hovered = isPointInRect((float)mx, (float)my,
            btnDialogClose.x, btnDialogClose.y, btnDialogClose.w, btnDialogClose.h);

        for (int i = 0; i < fileCount; i++) {
            if (files[i].y >= startY - itemH && files[i].y < DIALOG_HEIGHT - 20) {
                files[i].hovered = isPointInRect((float)mx, (float)my,
                    files[i].x, files[i].y, files[i].w, files[i].h);
            } else {
                files[i].hovered = 0;
            }
        }
    }

    if (event->type == RGFW_mouseButtonPressed && event->button.value == RGFW_mouseLeft) {

        if (btnDialogClose.hovered) {
            dialog_close();
            return;
        }

        for (int i = 0; i < fileCount; i++) {
            if (files[i].hovered) {
                if (files[i].is_dir) {
                    if (strcmp(files[i].name, "..") == 0) {
                       
                        char* lastSlash = strrchr(g_dialogPath, '/');
                        if (!lastSlash) lastSlash = strrchr(g_dialogPath, '\\');

                        if (lastSlash && lastSlash != g_dialogPath) {
                            *lastSlash = '\0';
#ifdef _WIN32
                            
                            size_t len = strlen(g_dialogPath);
                            if (len == 2 && g_dialogPath[1] == ':') {
                                g_dialogPath[2] = '\\';
                                g_dialogPath[3] = '\0';
                            }
#endif
                        } else if (lastSlash == g_dialogPath) {
                            g_dialogPath[1] = '\0';
                        } else {
                            strcpy(g_dialogPath, ".");
                        }
                    } else {
                        
                        char newPath[512];
                        if (strcmp(g_dialogPath, ".") == 0) {
                            snprintf(newPath, sizeof(newPath), "%s", files[i].name);
                        } else {
                            snprintf(newPath, sizeof(newPath), "%s" PATH_SEP "%s", g_dialogPath, files[i].name);
                        }
                        strncpy(g_dialogPath, newPath, 511);
                    }
                    scanDirectory(g_dialogPath);
                } else {
                    char fullPath[512];
                    if (strcmp(g_dialogPath, ".") == 0) {
                        snprintf(fullPath, sizeof(fullPath), "%s", files[i].name);
                    } else {
                        snprintf(fullPath, sizeof(fullPath), "%s" PATH_SEP "%s", g_dialogPath, files[i].name);
                    }

                    printf("Selected: %s\n", fullPath);

                    RGFW_window_makeCurrentContext_OpenGL(win);

                    if (g_avatar.isLoaded) {
                        UnloadAvatar(vg_win, &g_avatar);
                    }
                    LoadAvatarFromOra(vg_win, fullPath, &g_avatar);
                    RGFW_window_makeCurrentContext_OpenGL(dialog);
                    strncpy(g_config.defaultModelPath, fullPath, 511);
                    SaveConfig(g_configPath, &g_config);

                    dialog_close();
                }
                return;
            }
        }
    }

    if (event->type == RGFW_mouseScroll) {
        int maxScroll = (fileCount / cols) * (int)(itemH + gap) - (dialog->h - 100);
        if (maxScroll < 0) maxScroll = 0;

        if (event->scroll.y > 0) {
            dialog_scroll_offset -= 60;
            if (dialog_scroll_offset < 0) dialog_scroll_offset = 0;
        } else if (event->scroll.y < 0) {
            dialog_scroll_offset += 60;
            if (dialog_scroll_offset > maxScroll) dialog_scroll_offset = maxScroll;
        }
    }
}

void dialog_draw(void) {
    if (!dialog || !vg_dialog) return;

    RGFW_window_makeCurrentContext_OpenGL(dialog);
    glViewport(0, 0, dialog->w, dialog->h);
    glClearColor(ui_clear_color[0], ui_clear_color[1], ui_clear_color[2], ui_clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg_dialog, (float)dialog->w, (float)dialog->h, 1.0f);

    nvgFillColor(vg_dialog, ui_text_color);
    nvgFontSize(vg_dialog, 22.0f);
    nvgFontFace(vg_dialog, "sans");
    nvgTextAlign(vg_dialog, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(vg_dialog, 20, 12, "Select Avatar", NULL);

    drawButton(vg_dialog, &btnDialogClose);

    nvgFontSize(vg_dialog, 14.0f);
    nvgFillColor(vg_dialog, ui_text_color);
    nvgText(vg_dialog, 20, 42, g_dialogPath, NULL);

    float clipY = 60;
    float clipH = (float)dialog->h - 80;
    nvgSave(vg_dialog);
    nvgScissor(vg_dialog, 0, clipY, (float)dialog->w, clipH);

    float startY = 60;
    float itemW = 160;
    float itemH = 140;
    float gap = 12;
    
    int cols = (dialog->w - 40) / (int)(itemW + gap);
    if (cols < 1) cols = 1;

    for (int i = 0; i < fileCount; i++) {
        int col = i % cols;
        int row = i / cols;

        float x = 20 + col * (itemW + gap);
        float y = startY + row * (itemH + gap) - dialog_scroll_offset;

        if (y + itemH < clipY || y > clipY + clipH) continue;

        files[i].x = x;
        files[i].y = y;
        files[i].w = itemW;
        files[i].h = itemH;

        nvgBeginPath(vg_dialog);
        nvgRoundedRect(vg_dialog, x, y, itemW, itemH, 6.0f);

        if (files[i].hovered) {
             nvgFillColor(vg_dialog, ui_button_hover);
        } else {
             nvgFillColor(vg_dialog, ui_textbox_bg);
        }
        nvgFill(vg_dialog);

        nvgBeginPath(vg_dialog);
        nvgRoundedRect(vg_dialog, x, y, itemW, itemH, 6.0f);
        nvgStrokeColor(vg_dialog, ui_button_border);
        nvgStrokeWidth(vg_dialog, 1.0f);
        nvgStroke(vg_dialog);

        if (files[i].is_dir) {
            nvgFillColor(vg_dialog, ui_button_active);
            nvgFontSize(vg_dialog, 16.0f);
            nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(vg_dialog, x + itemW/2, y + itemH/2 - 15, "[DIR]", NULL);
        }
        else{
            if (files[i].thumbnailLoaded == 0) {
                char fullPath[512];
                if (strcmp(g_dialogPath, ".") == 0) {
                    snprintf(fullPath, sizeof(fullPath), "%s", files[i].name);
                } else {
                    snprintf(fullPath, sizeof(fullPath), "%s" PATH_SEP "%s", g_dialogPath, files[i].name);
                }
                files[i].thumbnailImage = loadThumbnailFromOra(vg_dialog, fullPath);
                files[i].thumbnailLoaded = (files[i].thumbnailImage > 0) ? 1 : -1;
            }

            if (files[i].thumbnailImage > 0) {
                int tw, th;
                nvgImageSize(vg_dialog, files[i].thumbnailImage, &tw, &th);

                float previewW = itemW - 16;
                float previewH = itemH - 35;
                float scale = fminf(previewW / tw, previewH / th);
                float dw = tw * scale;
                float dh = th * scale;
                float dx = x + (itemW - dw) / 2;
                float dy = y + 8;

                NVGpaint imgPaint = nvgImagePattern(vg_dialog, dx, dy, dw, dh,
                                                     0, files[i].thumbnailImage, 1.0f);
                nvgBeginPath(vg_dialog);
                nvgRoundedRect(vg_dialog, dx, dy, dw, dh, 4.0f);
                nvgFillPaint(vg_dialog, imgPaint);
                nvgFill(vg_dialog);
            } else {
                nvgFillColor(vg_dialog, ui_slider_track);
                nvgFontSize(vg_dialog, 14.0f);
                nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(vg_dialog, x + itemW/2, y + itemH/2 - 15, "[ORA]", NULL);
            }
        }

        nvgFillColor(vg_dialog, (files[i].hovered) ? ui_text_inv_color : ui_text_color);
        nvgFontSize(vg_dialog, 12.0f);
        nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);

        char displayName[32];
        if (strlen(files[i].name) > 20) {
            strncpy(displayName, files[i].name, 17);
            displayName[17] = '\0';
            strcat(displayName, "...");
        } else {
            strcpy(displayName, files[i].name);
        }
        nvgText(vg_dialog, x + itemW / 2, y + itemH - 8, displayName, NULL);
    }

    nvgRestore(vg_dialog);

    int maxScroll = (fileCount / cols) * (int)(itemH + gap) - (dialog->h - 100);
    if (maxScroll > 0) {
        nvgFillColor(vg_dialog, ui_text_color);
        nvgFontSize(vg_dialog, 11.0f);
        nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgText(vg_dialog, (float)dialog->w / 2, (float)dialog->h - 8, "Scroll for more", NULL);
    }

    nvgEndFrame(vg_dialog);
    glFlush();
    RGFW_window_swapBuffers_OpenGL(dialog);
}

#endif
