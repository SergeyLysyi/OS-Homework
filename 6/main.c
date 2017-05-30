#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define DEFAULT_PATH_TO_LOCKS "/tmp/"
#define DEFAULT_LOCKFILE_SUFFIX ".lock"

#define OPERATION_LITERAL_WRITE 'W'
#define OPERATION_LITERAL_READ 'R'

#define LOCKFILE_RECORD_FORMAT "%c %d\n"
// where %c - operation literal; %d - pid of creator

char* lockfile_suffix = DEFAULT_LOCKFILE_SUFFIX;
char* path_to_locks = DEFAULT_PATH_TO_LOCKS;

char* get_lock_file_path(char* file_to_edit_path) {
    size_t filepath_length = strlen(file_to_edit_path);
    int filename_starting_index = filepath_length;
    while(file_to_edit_path[filename_starting_index] != '/' && filename_starting_index>0){
        filename_starting_index--;
    }
    int substring_length = (int) filepath_length - filename_starting_index;
    char* lock_filepath = malloc(
        strlen(path_to_locks) + 
        substring_length + 
        strlen(lockfile_suffix));
    sprintf(lock_filepath, "%s%s%s", path_to_locks, file_to_edit_path+filename_starting_index, lockfile_suffix);

    return lock_filepath;
}

bool is_file_exists(char* path) {
    return access(path, F_OK) == 0 ? true : false;
}

void create_lock_file_by_path(char* path, char operation_literal) {
    FILE *fp = fopen(path, "wa");
    if (fp < 0) {
        fprintf(stderr, "Error: can't create lock file %s\n", path);
        exit(1);
    }
    fprintf(fp, LOCKFILE_RECORD_FORMAT, operation_literal, getpid());
    fclose(fp);
}

void remove_lock_file_by_path(char* path) {
    if (remove(path) < 0) {
        fprintf(stderr, "Error: can't remove lock file %s\n", path);
        exit(1);
    }
}

void append_to_file(char* file_to_edit_path, int argc, char *argv[]) {
    // simulates long write operation
    sleep(3);

    FILE *fp = fopen(file_to_edit_path, "a");
    int i;
    for (i = 1; i < argc - 2; i++) {
        fprintf(fp, "%s ", argv[i]);
    }
    fprintf(fp, "%s\n", argv[i]);
    fclose(fp);
}

void edit_file_with_lock(char* file_to_edit_path, int data_size, char *data[]) {
    char* lock_file = get_lock_file_path(file_to_edit_path);

    while (is_file_exists(lock_file)) {
        usleep(1);
    }

    create_lock_file_by_path(lock_file, OPERATION_LITERAL_WRITE);

    printf("writing...\n");
    append_to_file(file_to_edit_path, data_size, data);
    printf("done.\n");

    remove_lock_file_by_path(lock_file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <text to append> <path to file>\n", argv[0]);
        return 1;
    };

    char* file_to_edit = argv[argc - 1];
    edit_file_with_lock(file_to_edit, argc, argv);

    return 0;
}