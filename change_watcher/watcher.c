#include "watcher.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct FileState { 
    time_t last_mtime;
    off_t last_size;
};

FileState* create_file_state() {
    FileState* state = malloc(sizeof(FileState));
    state->last_mtime = 0;
    state->last_size = 0;
    return state;
}

void free_file_state(FileState* state) {
    free(state);
}

bool is_changed(FileState* state, FILE* file) {
    if (!file) {
        perror("File open failed");
        return false;
    }

    struct stat current_stat;
    if (fstat(fileno(file), &current_stat) == -1) {
        perror("fstat failed");
        return false;
    }

    if (state->last_mtime != current_stat.st_mtime || 
        state->last_size != current_stat.st_size) {
        state->last_mtime = current_stat.st_mtime;
        state->last_size = current_stat.st_size;
        return true;
    }
    return false;
}

off_t get_size(FileState* state, FILE* file) {
    // Not used in main solution but implemented for completeness
    if (!file) return -1;
    struct stat current_stat;
    if (fstat(fileno(file), &current_stat) == -1) return -1;
    return current_stat.st_size;
}

time_t get_time(FileState* state, FILE* file) {
    // Not used in main solution but implemented for completeness
    if (!file) return -1;
    struct stat current_stat;
    if (fstat(fileno(file), &current_stat) == -1) return -1;
    return current_stat.st_mtime;
}

