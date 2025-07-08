#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "proc.h"

double get_nvidia_gpu_usage() {
    FILE *fp = popen("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits", "r");
    if (!fp) {
        perror("popen");
        return -1.0;
    }
    char buffer[128];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        pclose(fp);
        return -1.0;
    }
    pclose(fp);

    double usage = atof(buffer);
    return usage;
}

double get_nvidia_gpu_temperature() {
    FILE *fp = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits", "r");
    if (!fp) {
        perror("popen");
        return -1.0;
    }

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        pclose(fp);
        return -1.0;
    }
    pclose(fp);

    return atof(buffer);
}

double get_amd_gpu_usage() {
    const char *base_path = "/sys/class/drm";
    DIR *dir = opendir(base_path);
    if (!dir) return -1.0;

    struct dirent *entry;
    char gpu_busy_path[256];
    double usage = -1.0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "card", 4) == 0) {
            snprintf(gpu_busy_path, sizeof(gpu_busy_path), "%s/%s/device/gpu_busy_percent", base_path, entry->d_name);
            FILE *fp = fopen(gpu_busy_path, "r");
            if (fp) {
                int val;
                if (fscanf(fp, "%d", &val) == 1) {
                    usage = (double)val;
                    fclose(fp);
                    break;
                }
                fclose(fp);
            }
        }
    }
    closedir(dir);
    return usage;
}

double get_amd_gpu_temperature() {
    const char *drm_path = "/sys/class/drm";
    DIR *dir = opendir(drm_path);
    if (!dir) return -1.0;

    struct dirent *entry;
    char temp_path[512];
    double temperature = -1.0;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "card", 4) == 0) {
            char hwmon_base[512];
            snprintf(hwmon_base, sizeof(hwmon_base), "%s/%s/device/hwmon", drm_path, entry->d_name);

            DIR *hwmon_dir = opendir(hwmon_base);
            if (!hwmon_dir) continue;

            struct dirent *hwmon_entry;
            while((hwmon_entry = readdir(hwmon_dir)) != NULL) {
                if (hwmon_entry->d_name[0] == '.') continue;
                snprintf(temp_path, sizeof(temp_path), "%s/%s/temp1_input", hwmon_base, hwmon_entry->d_name);
                FILE *fp = fopen(temp_path, "r");
                if (fp) {
                    int temp_milli;
                    if (fscanf(fp, "%d", &temp_milli) == 1) {
                        temperature = (double)temp_milli / 1000.0;
                        fclose(fp);
                        closedir(hwmon_dir);
                        closedir(dir);
                        return temperature;
                    }
                    fclose(fp);
                }
            }
            closedir(hwmon_dir);
        }
    }
    closedir(dir);
    return temperature;
}

double get_gpu_usage() {
    double usage = get_nvidia_gpu_usage();
    if (usage >= 0) {
        return usage;
    }
    usage = get_amd_gpu_usage();
    if (usage >= 0) {
        return usage;
    }
    return -1.0;
}

double get_gpu_temperature() {
    double temp = get_nvidia_gpu_temperature();
    if (temp >= 0) return temp;
    
    temp = get_amd_gpu_temperature();
    if (temp >= 0) return temp;

    return -1.0;
}