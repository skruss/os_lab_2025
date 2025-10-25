#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>


void print_process_info(const char* message) 
{
    printf("%s (PID: %d, PPID: %d)\n", message, getpid(), getppid());
}

void create_zombie() 
{
    pid_t pid = fork();
    
    if (pid == 0) 
    {
        // Дочерний процесс
        print_process_info("Дочерний процесс");
        printf("    Дочерний процесс работает 3 секунды...\n");
        sleep(3);
        print_process_info("Дочерний процесс завершается");
        exit(42);
    } 
    else 
    {
        // Родительский процесс
        print_process_info("Родительский процесс");
        printf("    Создал дочерний процесс с PID: %d\n", pid);
        printf("    НЕ вызываю wait() - это создаст зомби!\n\n");
        
        
        printf("Родительский процесс ждет 20 секунд...\n");
        for (int i = 1; i <= 20; i++) 
        {
            printf("   Прошло %d секунд...\n", i);
            sleep(1);
        }
        
        printf("\nРодительский процесс завершается\n");
        printf("   Зомби-процесс теперь исчезнет!\n");
    }
}

void prevent_zombie() 
{
    pid_t pid = fork();
    
    if (pid == 0) 
    {
        // Дочерний процесс
        print_process_info("Дочерний процесс");
        printf("    Дочерний процесс работает 3 секунды...\n");
        sleep(3);
        print_process_info("Дочерний процесс завершается");
        exit(42);
    } 
    else 
    {
        // Родительский процесс
        print_process_info("Родительский процесс");
        printf("Создал дочерний процесс с PID: %d\n", pid);
        printf("Вызываю wait() для предотвращения зомби!\n\n");
        
        int status;
        pid_t finished_pid = wait(&status);
        
        if (finished_pid == -1) 
        {
            perror("wait failed");
        } 
        else 
        {
            printf("Дочерний процесс %d завершился\n", finished_pid);
            if (WIFEXITED(status)) 
            {
                printf("Код возврата: %d\n", WEXITSTATUS(status));
            }
        }

    }
}


int main(int argc, char *argv[]) 
{
    printf("=== Демонстрация зомби-процессов ===\n\n");
    
    if (argc > 1 && strcmp(argv[1], "--zombie") == 0) 
    {
        create_zombie();
    } 
    else if (argc > 1 && strcmp(argv[1], "--prevent") == 0)
    {
        prevent_zombie();
    } 
    
    return 0;
}