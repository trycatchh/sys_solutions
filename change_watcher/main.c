#include "watcher.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    char *name;
    FileState *state;
    time_t last_changed;
    int is_new;
} FileEntry;

int is_file_watched(FileEntry *files, int count, const char *filename) {
    for (int i = 0; i < count; i++) {
        if (strcmp(files[i].name, filename) == 0) {
            return 1;
        }
    }
    return 0;
}

int scan_for_new_files(FileEntry **files, int *count, const char *directory) {
    DIR *dir = opendir(directory ? directory : ".");
    if (!dir) return 0;
    
    struct dirent *entry;
    int new_files_added = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
            continue;
        
        char full_path[1024];
        if (directory && strcmp(directory, ".") != 0) {
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);
        } else {
            strcpy(full_path, entry->d_name);
        }
        
        struct stat st;
        if (stat(full_path, &st) == -1) continue;
        if (!S_ISREG(st.st_mode)) continue;
        
        if (!is_file_watched(*files, *count, full_path)) {
            *files = realloc(*files, (*count + 1) * sizeof(FileEntry));
            (*files)[*count].name = strdup(full_path);
            (*files)[*count].state = create_file_state();
            (*files)[*count].last_changed = 0;
            (*files)[*count].is_new = 1;  
            (*count)++;
            new_files_added++;
        }
    }
    
    closedir(dir);
    return new_files_added;
}

int main(int argc, const char *argv[]) {
    const char *watch_directory = (argc < 2) ? "." : argv[1];
    
    DIR *test_dir = opendir(watch_directory);
    if (!test_dir) {
        perror("opendir failed");
        return 1;
    }
    closedir(test_dir);
    
    FileEntry *files = NULL;
    int count = 0;
    
    scan_for_new_files(&files, &count, (argc < 2) ? NULL : argv[1]);
    
    for (int i = 0; i < count; i++) {
        files[i].is_new = 0;
    }
    
    system("clear");
    
    int rescan_counter = 0;
    const int RESCAN_INTERVAL = 5; // Rescan every 5 seconds
    int screen_needs_refresh = 1;
    
    while (1) {
        time_t current_time = time(NULL);
        
        if (rescan_counter % RESCAN_INTERVAL == 0) {
            int new_files = scan_for_new_files(&files, &count, (argc < 2) ? NULL : argv[1]);
            if (new_files > 0) {
                screen_needs_refresh = 1;
            }
        }
        
        if (screen_needs_refresh) {
            system("clear");
            printf("Watching %d files in '%s'...\n\n", count, watch_directory);
            for (int i = 0; i < count; i++) {
                printf("%s | Size: calculating...\n", files[i].name);
            }
            screen_needs_refresh = 0;
        }
        
        printf("\033[%dA", count);
        
        for (int i = 0; i < count; i++) {
            FILE *file = fopen(files[i].name, "rb");
            if (!file) {
                printf("\033[K%s | Error: Cannot open file", files[i].name);
                if (files[i].is_new) {
                    printf(" [NEW]");
                }
                printf("\n");
                continue;
            }
            
            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            char *buffer = malloc(size + 1);
            size_t read = fread(buffer, 1, size, file);
            buffer[read] = '\0';
            
            printf("\033[K");
            
            printf("%s | Size: %ld bytes", files[i].name, size);
            
            if (is_changed(files[i].state, file)) {
                files[i].last_changed = current_time;
                printf(" [CHANGED]");
            } else if (current_time - files[i].last_changed < 3 && files[i].last_changed > 0) {
                printf(" [CHANGED]");
            }
            
            if (files[i].is_new) {
                printf(" [NEW]");
                if (current_time - files[i].last_changed > 5) {
                    files[i].is_new = 0; 
                }
            }
            
            printf("\n");
            
            free(buffer);
            fclose(file);
        }
        
        fflush(stdout);
        sleep(1);
        rescan_counter++;
    }
    
    for (int i = 0; i < count; i++) {
        free(files[i].name);
        free_file_state(files[i].state);
    }
    free(files);
    return 0;
}
