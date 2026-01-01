#define NOB_IMPLEMENTATION
#include "LIB/nob.h"

#define BUILD_FOLDER "build/"

bool embed_resource(Nob_String_Builder *sb, const char *file_path, const char *var_name)
{
    Nob_String_Builder content = {0};
    if (!nob_read_entire_file(file_path, &content)) return false;

    nob_sb_appendf(sb, "unsigned char %s[] = {", var_name);
    for (size_t i = 0; i < content.count; ++i) {
        nob_sb_appendf(sb, "0x%02x, ", (unsigned char)content.items[i]);
    }
    nob_sb_appendf(sb, "};\n");
    nob_sb_appendf(sb, "unsigned int %s_len = %zu;\n", var_name, content.count);

    nob_sb_free(content);
    return true;
}

bool build(void)
{
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return false;

    Nob_String_Builder sb = {0};
    if (!embed_resource(&sb, "RES/LiberStruct.ttf", "font_data")) return false;
    if (!nob_write_entire_file(BUILD_FOLDER "font.h", sb.items, sb.count)) return false;
    nob_sb_free(sb);

    Nob_Cmd cmd = {0};
    #ifdef _WIN32

        const char *icon_rc = "101 ICON \"RES/pngora.ico\"";
        if (!nob_write_entire_file(BUILD_FOLDER "icon.rc", icon_rc, strlen(icon_rc))) return false;

        #ifdef _MSC_VER
            nob_cmd_append(&cmd, "rc.exe", "/nologo", "/fo", BUILD_FOLDER "icon.res", BUILD_FOLDER "icon.rc");
            if (!nob_cmd_run_opt(&cmd, (Nob_Cmd_Opt){0})) return false;
            cmd.count = 0;
            nob_cmd_append(&cmd, "cl.exe", "/nologo", "/W3", "/O2", "/D_CRT_SECURE_NO_WARNINGS",
                           "SRC/main.c", "LIB/nanovgXC/nanovg.c", "LIB/glad/glad.c", "LIB/miniz.c",
                           BUILD_FOLDER "icon.res",
                           "/ILIB", "/ISRC", "/ILIB/stb", "/Ibuild",
                           "/Fe:" BUILD_FOLDER "PNGTuberORA.exe",
                           "/link", "opengl32.lib", "gdi32.lib", "user32.lib", "shell32.lib",
                           "ole32.lib", "winmm.lib");
        #else
            nob_cmd_append(&cmd, "windres", BUILD_FOLDER "icon.rc", "-o", BUILD_FOLDER "icon.o");
            if (!nob_cmd_run(&cmd)) return false;
            cmd.count = 0;
            nob_cmd_append(&cmd, "cc", "-O2", "SRC/main.c", "LIB/nanovgXC/nanovg.c", "LIB/glad/glad.c", "LIB/miniz.c",
                           BUILD_FOLDER "icon.o",
                           "-ILIB", "-ISRC", "-ILIB/stb", "-Ibuild",
                           "-lopengl32", "-lgdi32",// "-lole32", "-lwinmm",
                           "-o", BUILD_FOLDER "PNGTuberORA");
        #endif
    #elif defined(__APPLE__)
        // macOS
        nob_cmd_append(&cmd, "cc", "-O2", "SRC/main.c", "LIB/nanovgXC/nanovg.c", "LIB/glad/glad.c", "LIB/miniz.c",
                       "-ILIB", "-ISRC", "-ILIB/stb", "-Ibuild",
                       "-framework", "Cocoa",
                       "-framework", "CoreVideo",
                       "-framework", "OpenGL",
                       "-framework", "IOKit",
                       "-framework", "CoreAudio",
                       "-framework", "AudioToolbox",
                       "-o", BUILD_FOLDER "PNGTuberORA");
    #else
        // Linux
        nob_cmd_append(&cmd, "cc", "-O2", "SRC/main.c", "LIB/nanovgXC/nanovg.c", "LIB/glad/glad.c", "LIB/miniz.c",
                       "-ILIB", "-ISRC", "-ILIB/stb", "-Ibuild",
                       "-lX11", "-lGL", "-lXrandr", "-lm", "-lpthread", "-ldl",
                       "-o", BUILD_FOLDER "PNGTuberORA");
    #endif
    if (!nob_cmd_run_opt(&cmd, (Nob_Cmd_Opt){0})) return false;

    #ifdef _WIN32
        nob_delete_file(BUILD_FOLDER "font.h");
        nob_delete_file(BUILD_FOLDER "icon.rc");
        #ifdef _MSC_VER
            nob_delete_file(BUILD_FOLDER "icon.res");
        #else
            nob_delete_file(BUILD_FOLDER "icon.o");
        #endif
    #endif

    return true;
}




int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    return build() ? 0 : 1;
}
