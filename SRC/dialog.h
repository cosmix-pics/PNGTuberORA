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
static Button btnDialogClose = { 460, 10, 80, 28, "Cancel", 0 };
static int dialog_scroll_offset = 0;

// Extract thumbnail from ORA file
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

// Cleanup all loaded thumbnails
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
    // Cleanup previous thumbnails before scanning new directory
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
    // Deactivate current GL context before creating new window
    DeactivateCurrentGLContext();

    // Convert path to absolute if it's relative
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
    printf("[DEBUG] Dialog opening with path: %s\n", g_dialogPath);

    dialog = RGFW_createWindow("Select Avatar", 150, 150, DIALOG_WIDTH, DIALOG_HEIGHT,
                                RGFW_windowOpenGL | RGFW_windowNoResize);
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
        // Cleanup thumbnails before closing
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

    // Grid layout constants - larger items for thumbnails
    float startY = 60;
    float itemW = 160;
    float itemH = 140;  // Increased for thumbnail
    float gap = 12;
    int cols = 3;

    // Update file entry positions for hit testing
    for (int i = 0; i < fileCount; i++) {
        int col = i % cols;
        int row = i / cols;

        files[i].x = 20 + col * (itemW + gap);
        files[i].y = startY + row * (itemH + gap) - dialog_scroll_offset;
        files[i].w = itemW;
        files[i].h = itemH;
    }

    if (event->type == RGFW_mousePosChanged) {
        // Update hover states
        btnDialogClose.hovered = isPointInRect((float)mx, (float)my,
            btnDialogClose.x, btnDialogClose.y, btnDialogClose.w, btnDialogClose.h);

        for (int i = 0; i < fileCount; i++) {
            // Only hover if within visible area
            if (files[i].y >= startY - itemH && files[i].y < DIALOG_HEIGHT - 20) {
                files[i].hovered = isPointInRect((float)mx, (float)my,
                    files[i].x, files[i].y, files[i].w, files[i].h);
            } else {
                files[i].hovered = 0;
            }
        }
    }

    if (event->type == RGFW_mouseButtonPressed && event->button.value == RGFW_mouseLeft) {
        printf("[DEBUG] Left click at (%d, %d), fileCount=%d\n", mx, my, fileCount);

        // Debug: print first few file positions
        for (int d = 0; d < fileCount && d < 3; d++) {
            printf("[DEBUG] files[%d]: '%s' at (%.0f,%.0f) size (%.0f,%.0f) hovered=%d\n",
                   d, files[d].name, files[d].x, files[d].y, files[d].w, files[d].h, files[d].hovered);
        }

        // Close button
        if (btnDialogClose.hovered) {
            dialog_close();
            return;
        }

        // Check file entries
        for (int i = 0; i < fileCount; i++) {
            if (files[i].hovered) {
                printf("[DEBUG] Clicked on: %s (is_dir=%d)\n", files[i].name, files[i].is_dir);
                if (files[i].is_dir) {
                    // Navigate into directory
                    if (strcmp(files[i].name, "..") == 0) {
                        printf("[DEBUG] Going up from: %s\n", g_dialogPath);
                        // Go up
                        char* lastSlash = strrchr(g_dialogPath, '/');
                        if (!lastSlash) lastSlash = strrchr(g_dialogPath, '\\');

                        if (lastSlash && lastSlash != g_dialogPath) {
                            *lastSlash = '\0';
#ifdef _WIN32
                            // Handle Windows drive root (e.g., C: -> C:\)
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
                        // Go down
                        char newPath[512];
                        if (strcmp(g_dialogPath, ".") == 0) {
                            snprintf(newPath, sizeof(newPath), "%s", files[i].name);
                        } else {
                            snprintf(newPath, sizeof(newPath), "%s" PATH_SEP "%s", g_dialogPath, files[i].name);
                        }
                        strncpy(g_dialogPath, newPath, 511);
                    }
                    printf("[DEBUG] New path: %s\n", g_dialogPath);
                    scanDirectory(g_dialogPath);
                    printf("[DEBUG] After scan, fileCount=%d\n", fileCount);
                } else {
                    // Build full path and load avatar
                    char fullPath[512];
                    if (strcmp(g_dialogPath, ".") == 0) {
                        snprintf(fullPath, sizeof(fullPath), "%s", files[i].name);
                    } else {
                        snprintf(fullPath, sizeof(fullPath), "%s" PATH_SEP "%s", g_dialogPath, files[i].name);
                    }

                    printf("Selected: %s\n", fullPath);

                    // CRITICAL: Switch to main window GL context for avatar operations
                    // Avatar images are bound to vg_win's context, not dialog's context
                    RGFW_window_makeCurrentContext_OpenGL(win);

                    // Unload current avatar if loaded
                    if (g_avatar.isLoaded) {
                        UnloadAvatar(vg_win, &g_avatar);
                    }

                    // Load new avatar
                    LoadAvatarFromOra(vg_win, fullPath, &g_avatar);

                    // Switch back to dialog context
                    RGFW_window_makeCurrentContext_OpenGL(dialog);

                    // Update config and save
                    strncpy(g_config.defaultModelPath, fullPath, 511);
                    SaveConfig(g_configPath, &g_config);

                    dialog_close();
                }
                return;
            }
        }
    }

    // Mouse wheel scrolling
    if (event->type == RGFW_mouseScroll) {
        int maxScroll = (fileCount / 3) * (int)(itemH + gap) - (DIALOG_HEIGHT - 100);
        if (maxScroll < 0) maxScroll = 0;

        if (event->scroll.y > 0) {
            // Scroll up
            dialog_scroll_offset -= 60;
            if (dialog_scroll_offset < 0) dialog_scroll_offset = 0;
        } else if (event->scroll.y < 0) {
            // Scroll down
            dialog_scroll_offset += 60;
            if (dialog_scroll_offset > maxScroll) dialog_scroll_offset = maxScroll;
        }
    }
}

void dialog_draw(void) {
    if (!dialog || !vg_dialog) return;

    RGFW_window_makeCurrentContext_OpenGL(dialog);
    glViewport(0, 0, dialog->w, dialog->h);
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg_dialog, (float)dialog->w, (float)dialog->h, 1.0f);

    // Title
    nvgFillColor(vg_dialog, UI_TEXT_COLOR);
    nvgFontSize(vg_dialog, 22.0f);
    nvgFontFace(vg_dialog, "sans");
    nvgTextAlign(vg_dialog, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(vg_dialog, 20, 12, "Select Avatar", NULL);

    // Close button
    drawButton(vg_dialog, &btnDialogClose);

    // Current path label
    nvgFontSize(vg_dialog, 14.0f);
    nvgFillColor(vg_dialog, nvgRGBA(180, 180, 180, 255));
    nvgText(vg_dialog, 20, 42, g_dialogPath, NULL);

    // Clip area for file grid
    float clipY = 60;
    float clipH = DIALOG_HEIGHT - 80;
    nvgSave(vg_dialog);
    nvgScissor(vg_dialog, 0, clipY, (float)DIALOG_WIDTH, clipH);

    // Grid layout - larger items for thumbnails
    float startY = 60;
    float itemW = 160;
    float itemH = 140;  // Increased for thumbnail
    float gap = 12;
    int cols = 3;

    for (int i = 0; i < fileCount; i++) {
        int col = i % cols;
        int row = i / cols;

        float x = 20 + col * (itemW + gap);
        float y = startY + row * (itemH + gap) - dialog_scroll_offset;

        // Skip if outside visible area
        if (y + itemH < clipY || y > clipY + clipH) continue;

        // Store layout for hit testing
        files[i].x = x;
        files[i].y = y;
        files[i].w = itemW;
        files[i].h = itemH;

        // Draw box background
        nvgBeginPath(vg_dialog);
        nvgRoundedRect(vg_dialog, x, y, itemW, itemH, 6.0f);

        if (files[i].hovered) {
            if (files[i].is_dir) {
                nvgFillColor(vg_dialog, nvgRGBA(80, 80, 110, 255));
            } else {
                nvgFillColor(vg_dialog, nvgRGBA(110, 60, 85, 255));
            }
        } else {
            if (files[i].is_dir) {
                nvgFillColor(vg_dialog, nvgRGBA(55, 55, 75, 255));
            } else {
                nvgFillColor(vg_dialog, nvgRGBA(75, 40, 55, 255));
            }
        }
        nvgFill(vg_dialog);

        // Draw border
        nvgBeginPath(vg_dialog);
        nvgRoundedRect(vg_dialog, x, y, itemW, itemH, 6.0f);
        nvgStrokeColor(vg_dialog, nvgRGBA(100, 100, 100, 150));
        nvgStrokeWidth(vg_dialog, 1.0f);
        nvgStroke(vg_dialog);

        if (files[i].is_dir) {
            // Directory: show folder icon
            nvgFillColor(vg_dialog, nvgRGBA(200, 200, 100, 255));
            nvgFontSize(vg_dialog, 16.0f);
            nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(vg_dialog, x + itemW/2, y + itemH/2 - 15, "[DIR]", NULL);
        } else {
            // ORA file: load and display thumbnail
            if (files[i].thumbnailLoaded == 0) {
                // Lazy load thumbnail on first render
                char fullPath[512];
                if (strcmp(g_dialogPath, ".") == 0) {
                    snprintf(fullPath, sizeof(fullPath), "%s", files[i].name);
                } else {
                    snprintf(fullPath, sizeof(fullPath), "%s" PATH_SEP "%s", g_dialogPath, files[i].name);
                }
                files[i].thumbnailImage = loadThumbnailFromOra(vg_dialog, fullPath);
                files[i].thumbnailLoaded = (files[i].thumbnailImage > 0) ? 1 : -1;
            }

            // Draw thumbnail if available
            if (files[i].thumbnailImage > 0) {
                int tw, th;
                nvgImageSize(vg_dialog, files[i].thumbnailImage, &tw, &th);

                // Scale to fit in preview area (centered, aspect-preserved)
                float previewW = itemW - 16;
                float previewH = itemH - 35;  // Leave room for filename
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
                // No thumbnail: show placeholder
                nvgFillColor(vg_dialog, nvgRGBA(150, 100, 130, 200));
                nvgFontSize(vg_dialog, 14.0f);
                nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(vg_dialog, x + itemW/2, y + itemH/2 - 15, "[ORA]", NULL);
            }
        }

        // Draw filename at bottom
        nvgFillColor(vg_dialog, UI_TEXT_COLOR);
        nvgFontSize(vg_dialog, 12.0f);
        nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);

        // Truncate long names
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

    // Scroll indicator if needed
    int maxScroll = (fileCount / 3) * (int)(itemH + gap) - (DIALOG_HEIGHT - 100);
    if (maxScroll > 0) {
        nvgFillColor(vg_dialog, nvgRGBA(150, 150, 150, 180));
        nvgFontSize(vg_dialog, 11.0f);
        nvgTextAlign(vg_dialog, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgText(vg_dialog, DIALOG_WIDTH / 2, DIALOG_HEIGHT - 8, "Scroll for more", NULL);
    }

    nvgEndFrame(vg_dialog);
    glFlush();
    RGFW_window_swapBuffers_OpenGL(dialog);
}

#endif
