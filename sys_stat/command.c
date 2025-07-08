#include <stdio.h>
#include <string.h>
#include "command.h"

Command commands[] = {
    { "-v", "Application version.", "Version " VERSION "\n" },
    { "-h", "Learn commands.", NULL },
    { "-ms", "Change delay time. (millisecond)", NULL }
};

const int C_COUNT = sizeof(commands) / sizeof(commands[0]);

int findCommand(const char *cmd) {
    for (int i = 0; i < C_COUNT; i++) {
        if (strcmp(commands[i].command, cmd) == 0) {
            return i;
        }
    }
    return -1;
}

void printHelp(void) {
    printf("Commands & Usage:\n");
    for (int i = 0; i < C_COUNT; i++) {
        printf("  %s:\t%s\n", commands[i].command, commands[i].description);
    }
}