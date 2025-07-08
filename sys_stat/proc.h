#ifndef PROC_H
#define PROC_H

#include <stdio.h>

typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} CpuTimes;

int read_cpu_times(CpuTimes *times);

extern double get_cpu_usage();
extern double get_cpu_temperature();

extern double get_ram_usage();

extern double get_gpu_usage();
extern double get_gpu_temperature();

extern char *get_fs_type(const char *device);
extern void print_disk_info(const char *device, const char *mount_point, double swap_used_mb);
extern void list_disks();

extern const char* get_default_network_interface(void);
extern double get_download_speed_kbps(const char *iface, unsigned int interval_sec);
extern double get_upload_speed_kbps(const char *iface, unsigned int interval_sec);

#endif