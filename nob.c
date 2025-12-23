#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER   "src/"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    Nob_Cmd cmd = {0};
    #ifdef _WIN32
        const char *icon_rc = "101 ICON \"res/pngora.ico\"";
        if (!nob_write_entire_file("icon.rc", icon_rc, strlen(icon_rc))) return 1;
        nob_cmd_append(&cmd, "windres", "icon.rc", "-o", "icon.o");
        if (!nob_cmd_run(&cmd)) return 1;
        nob_cmd_append(&cmd, "cc", "icon.o", SRC_FOLDER"main.c", SRC_FOLDER"config.c", SRC_FOLDER"ora_loader.c", SRC_FOLDER"backend.c", "-lraylib", "-lgdi32", "-lwinmm", "-Iinc", "-Llib", "inc/miniz.c", "-o", BUILD_FOLDER"PNGTuberORA");
    #else
         nob_cmd_append(&cmd, "cc", SRC_FOLDER"main.c", SRC_FOLDER"config.c", SRC_FOLDER"ora_loader.c", SRC_FOLDER"backend.c","-lraylib", "-lGL", "-lm", "-lpthread", "-ldl", "-lrt", "-lX11", "-Iinc", "-Llib/linux", "inc/miniz.c", "-o", BUILD_FOLDER"PNGTuberORA");
    #endif
    
    if (!nob_cmd_run(&cmd)) return 1;

    #ifdef _WIN32
        nob_delete_file("icon.rc");
        nob_delete_file("icon.o");
    #endif
    
    return 0;
}
