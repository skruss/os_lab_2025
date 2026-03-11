#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "common.h"

struct Server {
    char ip[255];
    int port;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}

typedef struct {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
} ThreadData;

void *client_thread_func(void *arg) {
    ThreadData *d = (ThreadData *)arg;
    struct hostent *hostname = gethostbyname(d->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed for %s\n", d->server.ip);
        pthread_exit(NULL);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(d->server.port);
    server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sck);
        pthread_exit(NULL);
    }

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &d->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &d->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &d->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        perror("Send failed");
        close(sck);
        pthread_exit(NULL);
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        perror("Receive failed");
        close(sck);
        pthread_exit(NULL);
    }

    memcpy(&d->result, response, sizeof(uint64_t));
    close(sck);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers[255] = {'\0'};

    while (true) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                if (!ConvertStringToUI64(optarg, &k)) {
                    fprintf(stderr, "Invalid k\n");
                    return 1;
                }
                break;
            case 1:
                if (!ConvertStringToUI64(optarg, &mod)) {
                    fprintf(stderr, "Invalid mod\n");
                    return 1;
                }
                break;
            case 2:
                strncpy(servers, optarg, sizeof(servers) - 1);
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == (uint64_t)-1 || mod == (uint64_t)-1 || !strlen(servers)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
                argv[0]);
        return 1;
    }

    FILE *f = fopen(servers, "r");
    if (!f) {
        perror("fopen");
        return 1;
    }

    struct Server *servers_list = NULL;
    int servers_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0)
            continue;

        char *colon = strchr(line, ':');
        if (!colon) {
            fprintf(stderr, "Invalid line (no colon): %s\n", line);
            fclose(f);
            return 1;
        }
        *colon = '\0';
        char *ip = line;
        int port = atoi(colon + 1);

        servers_list = realloc(servers_list, sizeof(struct Server) * (servers_count + 1));
        strcpy(servers_list[servers_count].ip, ip);
        servers_list[servers_count].port = port;
        ++servers_count;
    }
    fclose(f);

    if (servers_count == 0) {
        fprintf(stderr, "No servers found in file\n");
        free(servers_list);
        return 1;
    }

    uint64_t chunk = (k + servers_count - 1) / servers_count;
    pthread_t *threads = malloc(sizeof(pthread_t) * servers_count);
    ThreadData *data = malloc(sizeof(ThreadData) * servers_count);

    for (int i = 0; i < servers_count; ++i) {
        data[i].server = servers_list[i];
        data[i].begin = 1 + i * chunk;
        data[i].end = data[i].begin + chunk - 1;
        if (data[i].end > k)
            data[i].end = k;
        data[i].mod = mod;
        data[i].result = 0;

        if (pthread_create(&threads[i], NULL, client_thread_func, &data[i]) != 0) {
            perror("pthread_create");
            free(threads);
            free(data);
            free(servers_list);
            return 1;
        }
    }

    uint64_t total = 1;
    for (int i = 0; i < servers_count; ++i) {
        pthread_join(threads[i], NULL);
        total = MultModulo(total, data[i].result, mod);
    }

    printf("Total: %" PRIu64 "\n", total);

    free(threads);
    free(data);
    free(servers_list);
    return 0;
}