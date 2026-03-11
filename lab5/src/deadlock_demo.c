#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_B = PTHREAD_MUTEX_INITIALIZER;

void* thread_1_func(void* arg) 
{
    printf("Поток 1: Попытка захватить мьютекс A...\n");
    pthread_mutex_lock(&mutex_A);
    printf("Поток 1: Захватил мьютекс A\n");

    usleep(100000);

    printf("Поток 1: Попытка захватить мьютекс B...\n");
    pthread_mutex_lock(&mutex_B); // deadlock
    printf("Поток 1: Захватил мьютекс B\n");

    // Эта часть никогда не выполнится
    pthread_mutex_unlock(&mutex_B);
    pthread_mutex_unlock(&mutex_A);
    return NULL;
}


void* thread_2_func(void* arg) 
{
    printf("Поток 2: Попытка захватить мьютекс B...\n");
    pthread_mutex_lock(&mutex_B);
    printf("Поток 2: Захватил мьютекс B\n");

    usleep(100000); // 100 мс

    printf("Поток 2: Попытка захватить мьютекс A...\n");
    pthread_mutex_lock(&mutex_A); // deadlock
    printf("Поток 2: Захватил мьютекс A\n");

    pthread_mutex_unlock(&mutex_A);
    pthread_mutex_unlock(&mutex_B);
    return NULL;
}

int main() 
{
    pthread_t thread1, thread2;

    printf("Запуск deadlock...\n");

    pthread_create(&thread1, NULL, thread_1_func, NULL);
    pthread_create(&thread2, NULL, thread_2_func, NULL);

    // Ожидание завершения потоков
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Программа завершена\n");
    return 0;
}