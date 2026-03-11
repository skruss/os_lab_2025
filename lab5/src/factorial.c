#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

typedef struct 
{
    int start;
    int end;
    int mod;
    long long *result;
    pthread_mutex_t *mutex;
} thread_arg_t;

void* thread_func(void* arg)
{
    thread_arg_t *targ = (thread_arg_t*) arg;
    int start = targ->start;
    int end = targ->end;
    int mod = targ->mod;
    long long prod = 1;

    for (int i = start; i <= end; ++i) 
    {
        prod = (prod * i) % mod;
    }

    // Захватываем мьютекс и обновляем результат
    pthread_mutex_lock(targ->mutex);
    *(targ->result) = (*(targ->result) * prod) % mod;
    pthread_mutex_unlock(targ->mutex);

    return NULL;
}

int main(int argc, char **argv) 
{
    int k = 0, pnum = 1, mod = 1;
    int opt;
    int option_index = 0;

    static struct option long_options[] = {
        {"pnum", required_argument, 0, 0},
        {"mod",  required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "k:", long_options, &option_index)) != -1) 
    {
        switch (opt) 
        {
            case 0:
                if (strcmp(long_options[option_index].name, "pnum") == 0) 
                {
                    pnum = atoi(optarg);
                } 
                else if (strcmp(long_options[option_index].name, "mod") == 0) 
                {
                    mod = atoi(optarg);
                }
                break;
            case 'k':
                k = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Использование: %s -k <число> --pnum=<число> --mod=<число>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (k <= 0 || pnum <= 0 || mod <= 0) 
    {
        fprintf(stderr, "Ошибка: все аргументы должны быть положительными числами.\n");
        exit(EXIT_FAILURE);
    }

    if (pnum > k) pnum = k;

    long long result = 1 % mod;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    pthread_t threads[pnum];
    thread_arg_t args[pnum];

    int base = k / pnum;
    int remainder = k % pnum;
    int current = 1;

    for (int i = 0; i < pnum; ++i) 
    {
        int start = current;
        int end = current + base - 1;
        if (i < remainder) end++;
        if (end > k) end = k;

        args[i].start = start;
        args[i].end = end;
        args[i].mod = mod;
        args[i].result = &result;
        args[i].mutex = &mutex;

        if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        current = end + 1;
    }

    for (int i = 0; i < pnum; ++i) 
    {
        pthread_join(threads[i], NULL);
    }

    printf("%lld\n", result);

    pthread_mutex_destroy(&mutex);
    return 0;
}