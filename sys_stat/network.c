#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "proc.h"

#define MAX_IFACE_NAME_LEN 128

typedef struct {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} NetStats;

const char* get_default_network_interface() {
    static char iface_name[MAX_IFACE_NAME_LEN];
    DIR *dir = opendir("/sys/class/net");
    if (!dir) return NULL;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (strcmp(entry->d_name, "lo") == 0) continue;

        char path[256];
        snprintf(path, sizeof(path), "/sys/class/net/%s/operstate", entry->d_name);

        FILE *fp = fopen(path, "r");
        if (!fp) continue;

        char state[16];
        if (fgets(state, sizeof(state), fp) != NULL) {
            state[strcspn(state, "\n")] = 0;
            fclose(fp);

            if (strcmp(state, "up") == 0) {
                strncpy(iface_name, entry->d_name, MAX_IFACE_NAME_LEN - 1);
                iface_name[MAX_IFACE_NAME_LEN - 1] = '\0';
                closedir(dir);
                return iface_name;
            }
        } else {
            fclose(fp);
        }
    }
    closedir(dir);
    return NULL;
}

int read_net_stats(const char *iface, NetStats *stats) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return -1;

    char line[512];
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        char *ptr = line;
        while (*ptr == ' ') ptr++;

        char iface_name[64];
        int i = 0;
        while (*ptr != ':' && *ptr != '\0' && i < (int)(sizeof(iface_name)-1)) {
            iface_name[i++] = *ptr++;
        }
        iface_name[i] = '\0';

        if (*ptr != ':') continue;
        ptr++;

        if (strcmp(iface_name, iface) == 0) {
            unsigned long long rx_bytes, tx_bytes;
            sscanf(ptr, "%llu %*s %*s %*s %*s %*s %*s %*s %llu", &rx_bytes, &tx_bytes);
            stats->rx_bytes = rx_bytes;
            stats->tx_bytes = tx_bytes;
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return -1;
}

double get_download_speed_kbps(const char *iface, unsigned int interval_sec) {
    NetStats stats1, stats2;
    if (read_net_stats(iface, &stats1) != 0) return -1.0;
    sleep(interval_sec);
    if (read_net_stats(iface, &stats2) != 0) return -1.0;

    if (stats2.rx_bytes < stats1.rx_bytes) return -1.0;

    unsigned long long diff = stats2.rx_bytes - stats1.rx_bytes;
    return (double)diff / 1024.0 / interval_sec;
}

double get_upload_speed_kbps(const char *iface, unsigned int interval_sec) {
    NetStats stats1, stats2;
    if (read_net_stats(iface, &stats1) != 0) return -1.0;
    sleep(interval_sec);
    if (read_net_stats(iface, &stats2) != 0) return -1.0;

    if (stats2.tx_bytes < stats1.tx_bytes) return -1.0;

    unsigned long long diff = stats2.tx_bytes - stats1.tx_bytes;
    return (double)diff / 1024.0 / interval_sec;
}
