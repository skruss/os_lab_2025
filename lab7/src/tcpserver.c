#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <bufsize>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int bufsize = atoi(argv[2]);

    const size_t kSize = sizeof(struct sockaddr_in);

    int lfd, cfd;
    int nread;
    char *buf = malloc(bufsize);
    if (!buf) {
        perror("malloc");
        exit(1);
    }

    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;

    if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        free(buf);
        exit(1);
    }

    memset(&servaddr, 0, kSize);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
        perror("bind");
        free(buf);
        exit(1);
    }

    if (listen(lfd, 5) < 0) {
        perror("listen");
        free(buf);
        exit(1);
    }

    printf("TCP server started on port %d, buffer size %d\n", port, bufsize);

    while (1) {
        unsigned int clilen = kSize;

        if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
            perror("accept");
            free(buf);
            exit(1);
        }
        printf("connection established\n");

        while ((nread = read(cfd, buf, bufsize)) > 0) {
            write(1, buf, nread);
        }

        if (nread == -1) {
            perror("read");
            free(buf);
            exit(1);
        }
        close(cfd);
    }

    free(buf);
    return 0;
}