#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAP_HEIGHT 10
#define MAP_WIDTH 10

#define INIT_MAP_FILENAME "init_map.map"
#define TEMP_MAP_FILENAME "temp_map.map"

#define DEAD '.'
#define ALIVE 'X'

char map[MAP_WIDTH][MAP_HEIGHT];

void print_map(char map[][MAP_HEIGHT]) {
    unsigned int i, j;
    printf("\n");
    for (i = 0; i < MAP_WIDTH; i++) {
        for (j = 0; j < MAP_HEIGHT; j++) {
            printf("%c", map[i][j]);
        }
        printf("\n");
    }
}

void init_map(char map[][MAP_HEIGHT]) {
    int read_from_file_failed = 0;

    FILE *fp = fopen(INIT_MAP_FILENAME, "r");
    if (fp == NULL) {
        fprintf(stderr, "no such file: unable to open map file %s\n", INIT_MAP_FILENAME);
        read_from_file_failed = 1;
    };

    int c;
    int i;
    int j;
    for (i = 0; i < MAP_WIDTH && read_from_file_failed == 0; i++) {
        for (j = 0; j < MAP_HEIGHT && read_from_file_failed == 0; j++) {
            c = fgetc(fp);
            switch(c) {
                case EOF:
                    fprintf(stderr, "wrong file format: unexpected EOF\n");
                    read_from_file_failed = 1;
                    break;
                case '\n':
                    fprintf(stderr, "wrong file format: unexpected new line\n");
                    read_from_file_failed = 1;
                    break;
                case '1':
                    map[i][j] = ALIVE;
                    break;
                case '0':
                    map[i][j] = DEAD;
                    break;
                default:
                    fprintf(stderr, "wrong file format: 10x10 chars filled with %c or %c\n", DEAD, ALIVE);
                    read_from_file_failed = 1;
                    break;
            }
        }
        fgetc(fp);
    }
    if (read_from_file_failed == 0){
        fclose(fp);
    }

    if (read_from_file_failed == 1){
        fprintf(stderr, "As read map from file failed, single glider created\n");
        for (i = 0; i < MAP_WIDTH; i++) {
            for (j = 0; j < MAP_HEIGHT; j++) {
                map[i][j] = DEAD;
            }
        }
        //glider
        //.X.
        //..X
        //XXX
        map[0][1] = ALIVE;
        map[1][2] = ALIVE;
        map[2][0] = ALIVE;
        map[2][1] = ALIVE;
        map[2][2] = ALIVE;
    }
}

int get_neighbors_number(char map[][MAP_HEIGHT], int x, int y) {
    int count = 0;

    for (int i = x-1; i<= x+1; i++){
        for (int j=y-1; j<=y+1; j++){
            if (i==x && y==j){
                continue;
            }
            if (i<0 || j<0 || i>=MAP_WIDTH || j>=MAP_HEIGHT){
                continue;
            }
            if (map[i][j] == ALIVE){
                count++;
            }
        }
    }

    return count;
}

void copy_map(char src[][MAP_HEIGHT], char dest[][MAP_HEIGHT]) {
    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_HEIGHT; j++) {
            dest[i][j] = src[i][j];
        }
    }
}

void next_generation(char map[][MAP_HEIGHT]) {
    int live_nb;
    char tmp_map[MAP_WIDTH][MAP_HEIGHT];

    time_t start_time = time(NULL);

    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_HEIGHT; j++) {
            live_nb = get_neighbors_number(map, i, j);

            if (map[i][j] == ALIVE) {
                if (live_nb < 2 || live_nb > 3) {
                    tmp_map[i][j] = DEAD;
                } else {
                    tmp_map[i][j] = ALIVE;
                }
            } else {
                if (live_nb == 3) {
                    tmp_map[i][j] = ALIVE;
                } else {
                    tmp_map[i][j] = DEAD;
                }
            }
        }
        time_t end_time = time(NULL);
        double calc_time = difftime(end_time, start_time);
        if (calc_time >= 1.0) {
            fprintf(stderr, "Warning: too long computation time. No response to client.\n");
            return;
        };
    }

    copy_map(tmp_map, map);
}

void timer_handler(int sig) {
    alarm(1);
    FILE *fp = fopen(TEMP_MAP_FILENAME, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: can't write to temp file\n");
        exit(3);
    };

    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_HEIGHT; j++) {
            if (fputc(map[i][j], fp) == EOF) {
                fprintf(stderr, "Error: can't write to temp file\n");
                exit(3);
            }
        }
        if (fputc('\n', fp) == EOF) {
            fprintf(stderr, "Error: can't write to temp file\n");
            exit(3);
        }
    }
    fclose(fp);
    next_generation(map);
    print_map(map);
}

void launch_game() {
    init_map(map);
    print_map(map);
    signal(SIGALRM, timer_handler);
    alarm(1);
    wait(NULL);
}

void launch_server(int port) {
    struct sockaddr_in serv_addr, cli_addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Error: unable to create socket\n");
        exit(404);
    };

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Error: unable to bind to socket at port %d\n", port);
        exit(404);
    };

    listen(sock, 1);
    socklen_t addr_size = sizeof(cli_addr);

    printf("Server launched\n");

    while(true) {
        int newsock = accept(sock, (struct sockaddr *)&cli_addr, &addr_size);
        if (newsock < 0) {
            fprintf(stderr, "Error: cant't accept connection\n");
            continue;
        }
        FILE *fp = fopen(TEMP_MAP_FILENAME, "r");
        if (fp != NULL) {
            int symbol;
            while((symbol = getc(fp)) != EOF) {
                int n = write(newsock, &symbol, 1);
                if (n < 0) {
                    fprintf(stderr, "Error: can't write to socket\n");
                    break;
                };
            }
            fclose(fp);
        }
        close(newsock);
    }
};


int main(int argc, char* argv[]) {
    if (argc < 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port;

    if (sscanf(argv[1] ,"%d", &port) < 1){
        printf("Port should be integer\n");
        exit(1);
    };

    int pid = fork();

    if (pid == -1){
        fprintf(stderr, "Error: can't start new process\n");
        exit(-1);
    }

    if (pid == 0){
        launch_server(port);
    } else {
        launch_game();
    }

    return 0;
}