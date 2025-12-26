#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER   "src/"
#define LINUXDEPLOY  BUILD_FOLDER "linuxdeploy-x86_64.AppImage"
#define LINUXDEPLOY_URL "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"

#ifdef _WIN32
    #ifdef _MSC_VER
        #define RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_win64_msvc16.zip"
        #define RAYLIB_DIR "raylib-5.0_win64_msvc16"
        #define RAYLIB_LIB_PATH "lib/raylib.lib"
    #else
        #define RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_win64_mingw-w64.zip"
        #define RAYLIB_DIR "raylib-5.0_win64_mingw-w64"
        #define RAYLIB_LIB_PATH "lib/libraylib.a"
    #endif
#else
    #define RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_linux_amd64.tar.gz"
    #define RAYLIB_DIR "raylib-5.0_linux_amd64"
    #define RAYLIB_LIB_PATH "lib/linux/libraylib.a"
#endif

bool download_raylib(void)
{
    if (nob_file_exists(RAYLIB_LIB_PATH)) return true;

    #ifdef _WIN32
    if (!nob_mkdir_if_not_exists("lib")) return false;
    #else
    if (!nob_mkdir_if_not_exists("lib")) return false;
    if (!nob_mkdir_if_not_exists("lib/linux")) return false;
    #endif

    nob_log(NOB_INFO, "Downloading raylib from %s...", RAYLIB_URL);
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "curl", "-L", "-o", "raylib.archive", RAYLIB_URL);
    if (!nob_cmd_run(&cmd)) return false;

    nob_log(NOB_INFO, "Extracting raylib...");
    cmd.count = 0;
    nob_cmd_append(&cmd, "tar", "-xf", "raylib.archive");
    if (!nob_cmd_run(&cmd)) return false;

    nob_log(NOB_INFO, "Installing raylib...");
    #ifdef _WIN32
        #ifdef _MSC_VER
            if (!nob_copy_file(RAYLIB_DIR "/lib/raylib.lib", RAYLIB_LIB_PATH)) return false;
        #else
            if (!nob_copy_file(RAYLIB_DIR "/lib/libraylib.a", RAYLIB_LIB_PATH)) return false;
        #endif
    #else
        if (!nob_copy_file(RAYLIB_DIR "/lib/libraylib.a", RAYLIB_LIB_PATH)) return false;
    #endif

    nob_log(NOB_INFO, "Cleaning up...");
    nob_delete_file("raylib.archive");
    
    cmd.count = 0;
    #ifdef _WIN32
    nob_cmd_append(&cmd, "cmd", "/c", "rmdir", "/s", "/q", RAYLIB_DIR);
    #else
    nob_cmd_append(&cmd, "rm", "-rf", RAYLIB_DIR);
    #endif
    if (!nob_cmd_run(&cmd)) return false;

    nob_cmd_free(cmd);
    return true;
}

bool build(void)
{
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return false;

    Nob_Cmd cmd = {0};
    #ifdef _WIN32
        if (!download_raylib()) return false;
        const char *icon_rc = "101 ICON \"res/pngora.ico\"";
        if (!nob_write_entire_file("icon.rc", icon_rc, strlen(icon_rc))) return false;

        #ifdef _MSC_VER
            nob_cmd_append(&cmd, "rc", "/fo", "icon.res", "icon.rc");
            if (!nob_cmd_run(&cmd)) return false;
            
            cmd.count = 0;
            nob_cmd_append(&cmd, "cl", "/nologo", "/O2", "/W3", "/MD", "/Iinc");
            nob_cmd_append(&cmd, SRC_FOLDER "main.c", SRC_FOLDER "config.c", SRC_FOLDER "ora_loader.c", SRC_FOLDER "backend.c", SRC_FOLDER "viseme_trainer.c", "inc/miniz.c");
            nob_cmd_append(&cmd, "/Fe" BUILD_FOLDER "PNGTuberORA.exe");
            nob_cmd_append(&cmd, "/link", "/LIBPATH:lib", "raylib.lib", "User32.lib", "Gdi32.lib", "Shell32.lib", "Winmm.lib", "icon.res");
        #else
            nob_cmd_append(&cmd, "windres", "icon.rc", "-o", "icon.o");
            if (!nob_cmd_run(&cmd)) return false;
            nob_cmd_append(&cmd, "cc", "icon.o",
                SRC_FOLDER "main.c", SRC_FOLDER "config.c", SRC_FOLDER "ora_loader.c", SRC_FOLDER "backend.c", SRC_FOLDER "viseme_trainer.c",
                "-lraylib", "-lgdi32", "-lwinmm", "-Iinc", "-Llib", "inc/miniz.c",
                "-o", BUILD_FOLDER "PNGTuberORA");
        #endif
    #else
        if (!download_raylib()) return false;
        nob_cmd_append(&cmd, "cc",
            SRC_FOLDER "main.c", SRC_FOLDER "config.c", SRC_FOLDER "ora_loader.c", SRC_FOLDER "backend.c", SRC_FOLDER "viseme_trainer.c",
            "-lraylib", "-lGL", "-lm", "-lpthread", "-ldl", "-lrt", "-lX11", "-Iinc", "-Llib/linux", "inc/miniz.c",
            "-o", BUILD_FOLDER "PNGTuberORA");
    #endif

    if (!nob_cmd_run(&cmd)) return false;

    #ifdef _WIN32
        nob_delete_file("icon.rc");
        #ifdef _MSC_VER
            nob_delete_file("icon.res");
        #else
            nob_delete_file("icon.o");
        #endif
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
