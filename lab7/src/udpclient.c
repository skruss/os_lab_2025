#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <IP> <port> <bufsize>\n", argv[0]);
        exit(1);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int bufsize = atoi(argv[3]);

    int sockfd, n;
    char *sendline = malloc(bufsize);
    char *recvline = malloc(bufsize + 1);
    if (!sendline || !recvline) {
        perror("malloc");
        exit(1);
    }

    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) < 0) {
        perror("inet_pton problem");
        free(sendline);
        free(recvline);
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket problem");
        free(sendline);
        free(recvline);
        exit(1);
    }

    write(1, "Enter string\n", 13);

    while ((n = read(0, sendline, bufsize)) > 0) {
        if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
            perror("sendto problem");
            free(sendline);
            free(recvline);
            exit(1);
        }

        if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
            perror("recvfrom problem");
            free(sendline);
            free(recvline);
            exit(1);
        }

        recvline[n] = 0;
        printf("REPLY FROM SERVER= %s\n", recvline);
    }

    close(sockfd);
    free(sendline);
    free(recvline);
    return 0;
}