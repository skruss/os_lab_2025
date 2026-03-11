#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

// Структура для передачи аргументов потоку
typedef struct {
    int start;              // начало диапазона
    int end;                // конец диапазона (включительно)
    int mod;                // модуль
    long long *result;      // указатель на общий результат
    pthread_mutex_t *mutex; // мьютекс для защиты результата
} thread_arg_t;

// Функция потока: вычисляет произведение чисел в своём диапазоне
// и добавляет его к общему результату под мьютексом
void* thread_func(void* arg) {
    thread_arg_t *targ = (thread_arg_t*) arg;
    int start = targ->start;
    int end = targ->end;
    int mod = targ->mod;
    long long prod = 1;

    // Вычисляем произведение по модулю для своего диапазона
    for (int i = start; i <= end; ++i) {
        prod = (prod * i) % mod;
    }

    // Захватываем мьютекс и обновляем общий результат
    pthread_mutex_lock(targ->mutex);
    *(targ->result) = (*(targ->result) * prod) % mod;
    pthread_mutex_unlock(targ->mutex);

    return NULL;
}

int main(int argc, char **argv) {
    int k = 0, pnum = 1, mod = 1;
    int opt;
    int option_index = 0;

    // Длинные опции: --pnum и --mod
    static struct option long_options[] = {
        {"pnum", required_argument, 0, 0},
        {"mod",  required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    // Разбор аргументов командной строки
    while ((opt = getopt_long(argc, argv, "k:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 0: // Длинная опция
                if (strcmp(long_options[option_index].name, "pnum") == 0) {
                    pnum = atoi(optarg);
                } else if (strcmp(long_options[option_index].name, "mod") == 0) {
                    mod = atoi(optarg);
                }
                break;
            case 'k': // Короткая опция -k
                k = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Использование: %s -k <число> --pnum=<число> --mod=<число>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Проверка корректности входных данных
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Ошибка: все аргументы должны быть положительными числами.\n");
        exit(EXIT_FAILURE);
    }

    // Корректировка числа потоков, если их больше, чем чисел
    if (pnum > k) pnum = k;

    // Инициализация общего результата и мьютекса
    long long result = 1 % mod;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    pthread_t threads[pnum];
    thread_arg_t args[pnum];

    // Распределение диапазонов между потоками
    int base = k / pnum;          // базовый размер диапазона
    int remainder = k % pnum;     // остаток, распределяемый между первыми потоками
    int current = 1;              // текущее начало диапазона

    for (int i = 0; i < pnum; ++i) {
        int start = current;
        int end = current + base - 1;
        if (i < remainder) end++; // первый remainder потоков получают на одно число больше
        if (end > k) end = k;     // защита от выхода за пределы

        args[i].start = start;
        args[i].end = end;
        args[i].mod = mod;
        args[i].result = &result;
        args[i].mutex = &mutex;

        // Создание потока
        if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        current = end + 1; // переходим к следующему диапазону
    }

    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Вывод результата
    printf("%lld\n", result);

    // Очистка
    pthread_mutex_destroy(&mutex);
    return 0;
}