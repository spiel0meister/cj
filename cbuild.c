#define CBUILD_IMPLEMENTATION
#include "cbuild.h"

int main(int argc, char** argv) {
    Cmd cmd = {};

    const char* cflags[] = STRS_LIT("-Wall", "-Wextra", "-ggdb");
    build_yourself(&cmd, argc, argv);

    if (!cmd_maybe_build_c(&cmd, CC_GCC, "cj", STRS("main.c"), cflags)) return 1;

    return 0;
}
