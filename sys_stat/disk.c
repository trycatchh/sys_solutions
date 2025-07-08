#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <stdbool.h>

#define MAX_LINE 1024
#define MAX_SWAP_DEVICES 16

#include "proc.h"
#include "ansi.h"

typedef struct {
    char device[64];
    double used_mb;
} SwapInfo;

char *get_fs_type(const char *device) {
    static char fs_type[64];
    fs_type[0] = '\0';

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "blkid -o value -s TYPE %s 2>/dev/null", device);

    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    if (fgets(fs_type, sizeof(fs_type), fp) == NULL) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);

    fs_type[strcspn(fs_type, "\n")] = 0;
    return fs_type;
}

void print_disk_info(const char *device, const char *mount_point, double swap_used_mb) {
    struct statvfs stat;
    if (statvfs(mount_point, &stat) != 0) {
        perror("statvfs");
        return;
    }

    unsigned long long free_bytes = stat.f_bavail * stat.f_frsize;
    unsigned long long total_bytes = stat.f_blocks * stat.f_frsize;

    double free_mb = (double)free_bytes / (1024 * 1024);
    double total_mb = (double)total_bytes / (1024 * 1024);
    double total_gb = (double)total_bytes / (1024 * 1024 * 1024);

    char *fs_type = get_fs_type(device);
    if (!fs_type) fs_type = "Unknown";

    if (swap_used_mb >= 0.01) {
        printf("%-20s %-15s " FG_GREEN "%10.2f MB" RESET " / " FG_YELLOW "%10.2f MB " RESET "/" BOLD " %6.2f GB" RESET " %-10s " FG_RED "[SWAP: %.2f MB]" RESET "\n",
               device, mount_point, free_mb, total_mb, total_gb, fs_type, swap_used_mb);
    } else {
        printf("%-20s %-15s " FG_GREEN "%10.2f MB" RESET " / " FG_YELLOW "%10.2f MB " RESET "/" BOLD " %6.2f GB" RESET " %-10s\n",
               device, mount_point, free_mb, total_mb, total_gb, fs_type);
    }
}

int read_swap_info(SwapInfo swap_list[], int max_swap) {
    FILE *fp = fopen("/proc/swaps", "r");
    if (!fp) {
        perror("fopen /proc/swaps");
        return 0;
    }

    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }

    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < max_swap) {
        char device[64], type[16];
        unsigned long size_kb, used_kb;
        int priority;

        if (sscanf(line, "%63s %15s %lu %lu %d", device, type, &size_kb, &used_kb, &priority) != 5) {
            continue;
        }

        strncpy(swap_list[count].device, device, sizeof(swap_list[count].device));
        swap_list[count].device[sizeof(swap_list[count].device) - 1] = '\0';
        swap_list[count].used_mb = used_kb / 1024.0;

        count++;
    }
    fclose(fp);
    return count;
}

double get_swap_usage_for_device(const char *device, SwapInfo swap_list[], int swap_count) {
    for (int i = 0; i < swap_count; i++) {
        if (strcmp(device, swap_list[i].device) == 0) {
            return swap_list[i].used_mb;
        }
    }
    return -1.0;
}

void list_disks() {
    SwapInfo swap_list[MAX_SWAP_DEVICES];
    int swap_count = read_swap_info(swap_list, MAX_SWAP_DEVICES);

    FILE *mnt_fp = setmntent("/proc/mounts", "r");
    if (!mnt_fp) {
        perror("setmntent");
        return;
    }

    struct mntent *mnt;

    printf(UNDERLINE "%-20s %-15s %-41s %-10s %s\n" RESET,
           "Device", "Mount Point", "Free Space", "FS Type", "Swap");

    while ((mnt = getmntent(mnt_fp)) != NULL) {
        if (strncmp(mnt->mnt_fsname, "/dev/", 5) != 0) continue;

        double swap_used = get_swap_usage_for_device(mnt->mnt_fsname, swap_list, swap_count);
        print_disk_info(mnt->mnt_fsname, mnt->mnt_dir, swap_used);
    }
    endmntent(mnt_fp);
}
