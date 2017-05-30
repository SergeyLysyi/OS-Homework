#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_RETRY_LAUNCH_ATTEMPTS 50

#define WAIT_AFTER_FAIL_MS 3600

#define DEFAULT_PID_FILES_FSTRING "/tmp/watcher.%d.pid"

#define COMMAND_SEP " "
#define KEYWORD_RESPAWN "respawn"
#define KEYWORD_WAIT "wait"

#define WP_MAX_PROCESS_LAUNCHED 100
#define WP_MAX_LENGTH_OF_COMMAND 100
#define WP_MAX_COMMAND_ARGUMENTS_AMOUNT 100
#define WP_MAX_FILENAME_LENGTH 100
#define WP_WAIT 0
#define WP_RESPAWN 1
#define HELP_MSG "Usage: %s <config filename>\n"

struct watcher_proc {
    pid_t pid;
    char command[WP_MAX_LENGTH_OF_COMMAND];
    char* command_args[WP_MAX_COMMAND_ARGUMENTS_AMOUNT];
    int watch_policy;
    int tries_count;
};

int watcher_procs_count = 0;
int proc_currently_launched = 0;
struct watcher_proc watcher_procs[WP_MAX_PROCESS_LAUNCHED];
char* conf_file_path;


char* get_file_name(int proc_idx) {
    char *file_name = (char*)malloc((size_t) WP_MAX_FILENAME_LENGTH);
    sprintf(file_name, DEFAULT_PID_FILES_FSTRING, watcher_procs[proc_idx].pid);
    return file_name;
}

void create_file(int proc_idx) {
    char *file_name = (char*)malloc((size_t) WP_MAX_FILENAME_LENGTH);
    strcpy(file_name, get_file_name(proc_idx));

    FILE *fp = fopen(file_name, "wa");

    if (fp == NULL) {
        syslog(LOG_ERR, "Can't open %s\n", file_name);
        exit(1);
    }

    fprintf(fp, "%d", watcher_procs[proc_idx].pid);
    fclose(fp);
    return;
}

void remove_file(int proc_idx) {
    char *file_name = (char*)malloc((size_t) WP_MAX_FILENAME_LENGTH);
    strcpy(file_name, get_file_name(proc_idx));

    if (remove(file_name) < 0) {
        syslog(LOG_ERR, "Can't delete %s\n", file_name);
        exit(1);
    }
    return;
}

void set_conf_file_path(int argc, char *argv[]) {
    if (argc != 2) {
        const char* err_msg = "Can't get conf file parameter";
        syslog(LOG_ERR, "%s\n", err_msg);
        fprintf(stderr, "%s\n", err_msg);
        printf(HELP_MSG, argv[0]);
        exit(1);
    }
    conf_file_path = argv[1];
    return;
}

void parse_conf() {
    FILE *fp = fopen(conf_file_path, "r");
    if (fp == NULL) {
        syslog(LOG_ERR, "Can't open conf file for reading\n");
        exit(1);
    };

    char* line = NULL;
    size_t len = 0;
    int iter = 0;

    while ((getline(&line, &len, fp)) != -1) {
        char sep[] = COMMAND_SEP;
        char* token = strtok(line, sep);

        bool was_compared = false;
        if (strncmp(token, KEYWORD_RESPAWN, 7) == 0) {
            watcher_procs[iter].watch_policy = WP_RESPAWN;
            was_compared = true;
        }
        if (strncmp(token, KEYWORD_WAIT, 4) == 0) {
            watcher_procs[iter].watch_policy = WP_WAIT;
            was_compared = true;
        }
        if (!was_compared) {
            syslog(LOG_ERR, "Invalid config file\n");
            exit(1);
        }

        watcher_procs[iter].tries_count = 0;
        watcher_procs[iter].pid = 0;

        char* args_token = strtok(NULL, sep);
        strcpy(watcher_procs[iter].command, args_token);
        for (int i = 0; i < WP_MAX_LENGTH_OF_COMMAND; ++i) {
            if (args_token != NULL) {
                for (int j = 0; j < strlen(args_token); ++j) {
                    if (args_token[j] == '\n') {
                        args_token[j] = '\0';
                        break;
                    }
                }

                watcher_procs[iter].command_args[i] = (char *)malloc(strlen(args_token));
                strcpy(watcher_procs[iter].command_args[i], args_token);
                args_token = strtok(NULL, sep);
            } else {
                watcher_procs[iter].command_args[i] = NULL;
                break;
            }
        }
        iter++;
    }

    watcher_procs_count = iter;
    fclose(fp);
    return;
}

void spawn(int proc_num, bool do_need_sleep) {
    int cpid = fork();
    switch (cpid) {
        case -1:
            syslog(LOG_ERR, "Can't fork for subproc\n");
            exit(1);
        case 0:
            if (do_need_sleep) {
                syslog(LOG_ERR, "Can't start %s\n", watcher_procs[proc_num].command);
                sleep(WAIT_AFTER_FAIL_MS);
                watcher_procs[proc_num].tries_count = 0;
            }

            if (execvp(watcher_procs[proc_num].command, watcher_procs[proc_num].command_args) < 0) {
                syslog(LOG_ERR, "Can't exec program %s with args: %s\n", watcher_procs[proc_num].command,
                       (char *) watcher_procs[proc_num].command_args);
                exit(1);
            }

            syslog(LOG_NOTICE, "%d created, pid = %d\n", proc_num, cpid);
            exit(0);
        default:
            watcher_procs[proc_num].pid = cpid;
            create_file(proc_num);
            proc_currently_launched++;
    }
}

void run_tasks() {
    watcher_procs_count = 0;
    proc_currently_launched = 0;

    parse_conf();

    for (int i = 0; i < watcher_procs_count; ++i) {
        spawn(i, false);
    }

    while (proc_currently_launched) {
        int status;
        pid_t cpid = wait(&status);

        for (int i = 0; i < watcher_procs_count; ++i) {
            if (watcher_procs[i].pid == cpid) {
                syslog(LOG_NOTICE, "command %d with pid %d was finished\n", i, cpid);
                remove_file(i);
                proc_currently_launched--;

                if (watcher_procs[i].watch_policy == WP_RESPAWN) {
                    if (status != 0) {
                        watcher_procs[i].tries_count++;
                    }

                    if (watcher_procs[i].tries_count >= MAX_RETRY_LAUNCH_ATTEMPTS) {
                        spawn(i, true);
                    } else {
                        spawn(i, false);
                    }
                }
            }
        }
    }

    syslog(LOG_INFO, "All tasks finished \n");
}

void sig_handler(int sig) {
    for (int i = 0; i < watcher_procs_count; i++) {
        if (watcher_procs[i].pid > 0) {
            kill(watcher_procs[i].pid, SIGKILL);
        }
    }
    run_tasks();
}


int main(int argc, char *argv[]) {
    signal(SIGHUP, (void (*)(int)) sig_handler);
    set_conf_file_path(argc, argv);

    int pid = fork();

    if (pid < 0) {
        syslog(LOG_ERR, "Can't start daemon\n");
        exit(1);
    }

    if (pid == 0){
        setsid();
        chdir("/");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        run_tasks();
    }

    return 0;
}