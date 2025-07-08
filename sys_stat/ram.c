#include <stdio.h>

#include "proc.h"

double get_ram_usage() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1.0;

    unsigned long long mem_total = 0, mem_free = 0, buffers = 0, cached = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu kB", &mem_total) == 1) continue;
        if (sscanf(line, "MemFree: %llu kB", &mem_free) == 1) continue;
        if (sscanf(line, "Buffers: %llu kB", &buffers) == 1) continue;
        if (sscanf(line, "Cached: %llu kB", &cached) == 1) continue;
    }

    fclose(fp);

    if (mem_total == 0) return -1.0;

    unsigned long long used = mem_total - (mem_free + buffers + cached);

    double usage = (double)used / mem_total * 100.0;
    return usage;
}