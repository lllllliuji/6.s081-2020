#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    char buf[512];
    char* args[MAXARG];
    char ch;
    int i;

    for (i = 0; i < argc - 1; i++) {
        args[i] = argv[i + 1];
    }

    for (;;) {
        i = 0;
        for(;;) {
            if (read(0, &ch, 1) == 0 || ch == '\n') {
                if (fork() == 0) {
                    buf[i] = 0;
                    args[argc - 1] = buf;
                    exec(args[0], args);
                    // never run here, except exec encounter an error
                    printf("here\n");
                }
                else {
                    wait(0);
                    break;
                }
            }
            else {
                buf[i++] = ch;
            }
        }
        if (i == 0) break;
    }
    exit(0);
}