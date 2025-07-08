#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "proc.h"

int read_cpu_times(CpuTimes *times) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return -1;

    int ret = fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &times->user, &times->nice, &times->system, &times->idle, &times->iowait, &times->irq, &times->softirq, &times->steal);
    fclose(fp);
    return (ret == 8) ? 0 : -1;
}

double get_cpu_usage() {
    CpuTimes t1, t2;
    if (read_cpu_times(&t1) != 0) return -1.0;

    sleep(1);

    if (read_cpu_times(&t2) != 0) return -1.0;

    unsigned long long idle1 = t1.idle + t1.iowait;
    unsigned long long idle2 = t2.idle + t2.iowait;

    unsigned long long nonIdle1 = t1.user + t1.nice + t1.system + t1.irq + t1.softirq + t1.steal;
    unsigned long long nonIdle2 = t2.user + t2.nice + t2.system + t2.irq + t2.softirq + t2.steal;

    unsigned long long total1 = idle1 + nonIdle1;
    unsigned long long total2 = idle2 + nonIdle2;
    
    unsigned long long totald = total2 - total1;
    unsigned long long idled = idle2 - idle1;

    if (totald == 0) return -1.0;

    double cpu_percentage = (double)(totald - idled) / totald * 100.0;
    return cpu_percentage;
}

double get_cpu_temperature() {
    const char *thermal_base = "/sys/class/thermal";
    DIR *dir = opendir(thermal_base);
    if (!dir) {
        perror("opendir");
        return -1.0;
    }

    struct dirent *entry;
    char temp_path[256];
    char type_path[256];
    char type[64];
    double temperature = -1.0;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "thermal_zone", 12) == 0) {
            snprintf(type_path, sizeof(type_path), "%s/%s/type", thermal_base, entry->d_name);
            FILE *type_fp = fopen(type_path, "r");
            if (!type_fp) continue;

            if (fgets(type, sizeof(type), type_fp) != NULL) {
                type[strcspn(type, "\n")] = 0;

                if (strstr(type, "cpu") || strstr(type, "CPU")) {
                    fclose(type_fp);

                    snprintf(temp_path, sizeof(temp_path), "%s/%s/temp", thermal_base, entry->d_name);
                    FILE *temp_fp = fopen(temp_path, "r");
                    if (!temp_fp) return -1.0;

                    int temp_milli;
                    if (fscanf(temp_fp, "%d", &temp_milli) == 1) {
                        temperature = temp_milli / 1000.0;
                        fclose(temp_fp);
                        closedir(dir);
                        return temperature;
                    }
                    fclose(temp_fp);
                    return -1.0;
                }
            }
            fclose(type_fp);
        }
    }

    snprintf(temp_path, sizeof(temp_path), "%s/thermal_zone0/temp", thermal_base);
    FILE *temp_fp = fopen(temp_path, "r");
    if (!temp_fp) {
        closedir(dir);
        return -1.0;
    }

    int temp_milli;
    if (fscanf(temp_fp, "%d", &temp_milli) == 1) {
        temperature = temp_milli / 1000.0;
    }
    fclose(temp_fp);
    closedir(dir);
    return temperature;
}