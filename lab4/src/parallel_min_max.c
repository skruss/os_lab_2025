#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Явное объявление kill если не объявлен в signal.h
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

int main(int argc, char **argv) 
{
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1;
  bool with_files = false;

  pid_t *child_pids = NULL;

  while (true) 
  {
    static struct option options[] = {
      {"seed", required_argument, 0, 0},
      {"array_size", required_argument, 0, 0},
      {"pnum", required_argument, 0, 0},
      {"by_files", no_argument, 0, 'f'},
      {"timeout", required_argument, 0, 0},
      {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) 
    {
      case 0:
        switch (option_index) 
        {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) 
            {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) 
            {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) 
            {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) 
            {
              printf("timeout must be a positive number\n");
              return 1;
            }
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) 
  {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) 
  {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  child_pids = malloc(pnum * sizeof(pid_t));
  if (child_pids == NULL) {
    perror("malloc failed");
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  int pipes[2 * pnum];
  if (!with_files) 
  {
    for (int i = 0; i < pnum; i++) 
    {
      if (pipe(pipes + i * 2) < 0) 
      {
        perror("pipe");
        free(child_pids);
        return 1;
      }
    }
  }

  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int chunk_size = array_size / pnum;

  for (int i = 0; i < pnum; i++) 
  {
    pid_t child_pid = fork();
    if (child_pid >= 0) 
    {
      active_child_processes += 1;
      if (child_pid == 0) 
      {
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : start + chunk_size;
        
        struct MinMax local_min_max = GetMinMax(array, start, end);

        if (with_files) 
        {
          char filename_min[20], filename_max[20];
          sprintf(filename_min, "min_%d.txt", i);
          sprintf(filename_max, "max_%d.txt", i);
          
          FILE *file_min = fopen(filename_min, "w");
          FILE *file_max = fopen(filename_max, "w");
          if (file_min && file_max) 
          {
            fprintf(file_min, "%d", local_min_max.min);
            fprintf(file_max, "%d", local_min_max.max);
            fclose(file_min);
            fclose(file_max);
          }
        } 
        else 
        {
          close(pipes[i * 2]);
          write(pipes[i * 2 + 1], &local_min_max.min, sizeof(int));
          write(pipes[i * 2 + 1], &local_min_max.max, sizeof(int));
          close(pipes[i * 2 + 1]);
        }
        free(array);
        exit(0);
      }
      else 
      {
        child_pids[i] = child_pid;
      }
    } 
    else 
    {
      printf("Fork failed!\n");
      free(child_pids);
      return 1;
    }
  }

  if (!with_files) 
  {
    for (int i = 0; i < pnum; i++) 
    {
      close(pipes[i * 2 + 1]);
    }
  }

  if (timeout > 0) 
  {
    int children_done = 0;
    struct timeval current_time;
    
    while (active_child_processes > 0) 
    {
      gettimeofday(&current_time, NULL);
      double elapsed_time = (current_time.tv_sec - start_time.tv_sec);
      
      if (elapsed_time >= timeout) 
      {
        printf("Timeout exceeded (%d seconds). Killing child processes...\n", timeout);
        for (int i = 0; i < pnum; i++) 
        {
          if (kill(child_pids[i], SIGKILL) == 0) 
          {
            printf("Killed child process with PID %d\n", child_pids[i]);
          }
        }
        break;
      }
      
      pid_t finished_pid = waitpid(-1, NULL, WNOHANG);
      if (finished_pid > 0) 
      {
        active_child_processes -= 1;
        children_done++;
        printf("Child process %d finished (%d/%d done)\n", 
               finished_pid, children_done, pnum);
      }
      
      sleep(1);
    }
  }
  else 
  {
    while (active_child_processes > 0) 
    {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  if (active_child_processes > 0) 
  {
    printf("Waiting for killed processes to terminate...\n");
    while (active_child_processes > 0) 
    {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) 
  {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) 
    {
      char filename_min[20], filename_max[20];
      sprintf(filename_min, "min_%d.txt", i);
      sprintf(filename_max, "max_%d.txt", i);
      
      FILE *file_min = fopen(filename_min, "r");
      FILE *file_max = fopen(filename_max, "r");
      if (file_min && file_max) 
      {
        fscanf(file_min, "%d", &min);
        fscanf(file_max, "%d", &max);
        fclose(file_min);
        fclose(file_max);
        remove(filename_min);
        remove(filename_max);
      }
    } 
    else 
    {
      int bytes_read_min = read(pipes[i * 2], &min, sizeof(int));
      int bytes_read_max = read(pipes[i * 2], &max, sizeof(int));
      
      if (bytes_read_min != sizeof(int) || bytes_read_max != sizeof(int)) 
      {
        continue;
      }
      close(pipes[i * 2]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  if (timeout > 0 && elapsed_time / 1000.0 >= timeout) 
  {
    printf("Warning: Execution was terminated by timeout\n");
  }
  
  fflush(NULL);
  return 0;
}