#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    if (argc < 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port;
    if (sscanf(argv[1] ,"%d", &port) < 1){
        printf("Port should be integer\n");
        exit(1);
    };

    struct sockaddr_in serv_addr;
    struct hostent *server;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Error: unable to create socket\n");
        exit(404);
    };

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Unable to conenct\n");
        exit(500);
    }

    char symb;
    while (read(sock, &symb, 1) > 0) {
        printf("%c", symb);
    };
    close(sock);

    return 0;

};