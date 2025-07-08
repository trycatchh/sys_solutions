#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "command.h"
#include "proc.h"

#include "ansi.h"

void run_app(double ms_buf);

int main(int argc, char *argv[]) {
    double ms_buf = .1f;

    if (argc > 1) {
        int idx = findCommand(argv[1]);
        if (idx >= 0) {
            if (commands[idx].output != NULL) {
                printf("%s", commands[idx].output);
            } else if (strcmp(commands[idx].command, "-h") == 0) {
                printHelp();
            } else if (strcmp(commands[idx].command, "-ms") == 0 && argc >= 3) {
                ms_buf = strtod(argv[2], NULL);
                run_app(ms_buf);
            } else {
                exit(EXIT_FAILURE);
            }
        } else {
            printf(FG_YELLOW "[WAR]" RESET " Unknown command: " FG_RED "%s" RESET "\nLearn commands: -h\n", argv[1]);
        }
    } else {
        // printf(YELLOW "[WAR]" RESET "No command provided.\nLearn commands: -h\n");
        run_app(ms_buf);
    }
    
    return 0;
}

void run_app(double ms_buf) {
    while (1) {
        // system("clear");
        // printf("\033[2J\033[H");
        // clrscr();
        printf("\e[1;1H\e[2J");
        printf("\033[H");
        
        double cpu = get_cpu_usage();
        double cpu_temp = get_cpu_temperature();

        double gpu = get_gpu_usage();
        double gpu_temp = get_gpu_temperature();

        double ram = get_ram_usage();
        if (cpu < 0) {
            printf(FG_RED "[ERR]" RESET "Failed to read CPU usage.\n");
        } else if (cpu_temp < 0) {
            printf(FG_RED "[ERR]" RESET "Could not read CPU temperature.\n");
        } else {
            printf(BG_BRIGHT_GREEN BOLD "CPU:" RESET " %.2f%% / %.2f°C\t", cpu, cpu_temp);
        }

        if (gpu < 0) {
            printf(FG_RED "[ERR]" RESET "Failed to get GPU usage or no NVIDIA GPU found.\n");
        } else if (gpu_temp < 0) {
            printf(FG_RED "[ERR]" RESET "GPU temperature not avaible.\n");
        } else {
            printf(BG_BRIGHT_YELLOW BOLD "GPU:" RESET " %.2f%% / %.2f°C\t", gpu, gpu_temp);
        }

        if (ram < 0) {
            printf(FG_RED "[ERR]" RESET "Failed to read RAM usage.\n");
        } else {
            printf(BG_BRIGHT_BLACK BOLD "RAM:" RESET " %.2f%%\t\t\t\t", ram);
        }

        printf("MS: %.1f\n", ms_buf);
        
        printf("\n");
        list_disks();
        printf("\n");

        const char *iface = get_default_network_interface();
        if (iface) {
            double download_kbps = get_download_speed_kbps(iface, 1);
            double upload_kbps = get_upload_speed_kbps(iface, 1);
            
            if (download_kbps < 0 || upload_kbps < 0) {
                printf(FG_RED "[ERR]" RESET "Can't read interface or error.");
            } else {
                printf(BG_BRIGHT_BLUE BOLD "DOWNLOAD:" RESET " %.2f KB/s\t", download_kbps);
                printf(BG_BRIGHT_BLUE BOLD "UPLOAD:" RESET " %.2f KB/s\n", upload_kbps);
            }
        } else {
            printf(FG_RED "[ERR]" RESET "Not found active network interface.");
        }

        fflush(stdout);
        sleep(ms_buf);
    }
}