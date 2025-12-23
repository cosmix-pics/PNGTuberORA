#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER   "src/"
#define LINUXDEPLOY  BUILD_FOLDER "linuxdeploy-x86_64.AppImage"
#define LINUXDEPLOY_URL "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"

bool build(void)
{
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return false;

    Nob_Cmd cmd = {0};
    #ifdef _WIN32
        const char *icon_rc = "101 ICON \"res/pngora.ico\"";
        if (!nob_write_entire_file("icon.rc", icon_rc, strlen(icon_rc))) return false;
        nob_cmd_append(&cmd, "windres", "icon.rc", "-o", "icon.o");
        if (!nob_cmd_run(&cmd)) return false;
        nob_cmd_append(&cmd, "cc", "icon.o",
            SRC_FOLDER "main.c", SRC_FOLDER "config.c", SRC_FOLDER "ora_loader.c", SRC_FOLDER "backend.c",
            "-lraylib", "-lgdi32", "-lwinmm", "-Iinc", "-Llib", "inc/miniz.c",
            "-o", BUILD_FOLDER "PNGTuberORA");
    #else
        nob_cmd_append(&cmd, "cc",
            SRC_FOLDER "main.c", SRC_FOLDER "config.c", SRC_FOLDER "ora_loader.c", SRC_FOLDER "backend.c",
            "-lraylib", "-lGL", "-lm", "-lpthread", "-ldl", "-lrt", "-lX11", "-Iinc", "-Llib/linux", "inc/miniz.c",
            "-o", BUILD_FOLDER "PNGTuberORA");
    #endif

    if (!nob_cmd_run(&cmd)) return false;

    #ifdef _WIN32
        nob_delete_file("icon.rc");
        nob_delete_file("icon.o");
    #endif

    return true;
}

#ifndef _WIN32
bool build_appimage(void)
{
    Nob_Cmd cmd = {0};

    // Download linuxdeploy if not present
    if (!nob_file_exists(LINUXDEPLOY)) {
        nob_log(NOB_INFO, "Downloading linuxdeploy...");
        nob_cmd_append(&cmd, "wget", "-q", "--show-progress", "-O", LINUXDEPLOY, LINUXDEPLOY_URL);
        if (!nob_cmd_run(&cmd)) return false;
        nob_cmd_append(&cmd, "chmod", "+x", LINUXDEPLOY);
        if (!nob_cmd_run(&cmd)) return false;
    }

    // Create AppDir structure
    nob_cmd_append(&cmd, "rm", "-rf", BUILD_FOLDER "AppDir");
    if (!nob_cmd_run(&cmd)) return false;

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER "AppDir")) return false;
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER "AppDir/usr")) return false;
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER "AppDir/usr/bin")) return false;

    // Copy executable
    if (!nob_copy_file(BUILD_FOLDER "PNGTuberORA", BUILD_FOLDER "AppDir/usr/bin/PNGTuberORA")) return false;

    // Run linuxdeploy (from build/ directory so AppImage is created there)
    nob_log(NOB_INFO, "Building AppImage...");
    if (!nob_set_current_dir(BUILD_FOLDER)) return false;
    nob_cmd_append(&cmd,
        "env", "DISABLE_COPYRIGHT_FILES_DEPLOYMENT=1", "NO_STRIP=true",
        "./linuxdeploy-x86_64.AppImage",
        "--appdir", "AppDir",
        "--desktop-file", "../res/pngora.desktop",
        "--icon-file", "../res/pngora.svg",
        "--output", "appimage");
    bool result = nob_cmd_run(&cmd);
    if (!nob_set_current_dir("..")) return false;

    if (result) {
        nob_log(NOB_INFO, "AppImage created: " BUILD_FOLDER "PNGTuberORA-x86_64.AppImage");
    }
    return result;
}
#endif

void usage(const char *program)
{
    nob_log(NOB_INFO, "Usage: %s [command]", program);
    nob_log(NOB_INFO, "Commands:");
    nob_log(NOB_INFO, "  (none)    Build the application");
    #ifndef _WIN32
        nob_log(NOB_INFO, "  appimage  Build an AppImage (Linux only)");
    #endif
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *program = nob_shift_args(&argc, &argv);

    if (argc == 0) {
        // Default: just build
        if (!build()) return 1;
    } else {
        const char *command = nob_shift_args(&argc, &argv);
#ifndef _WIN32
        if (strcmp(command, "appimage") == 0) {
            if (!build()) return 1;
            if (!build_appimage()) return 1;
        } else
#endif
        {
            nob_log(NOB_ERROR, "Unknown command: %s", command);
            usage(program);
            return 1;
        }
    }

    return 0;
}
