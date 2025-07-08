#ifndef WATCHER_H
#define WATCHER_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef struct FileState FileState;

FileState* create_file_state();
void free_file_state(FileState* state);

bool is_changed(FileState* state, FILE* file);
off_t get_size(FileState* state, FILE* file);
time_t get_time(FileState* state, FILE* file);

#endif
