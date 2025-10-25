#include "sum.h"
#include <pthread.h>

struct SumArgs {
  int *array;
  int begin;
  int end;
};

static int Sum(const struct SumArgs *args) {
  int sum = 0;
  for (int i = args->begin; i < args->end; i++) {
    sum += args->array[i];
  }
  return sum;
}

static void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int parallel_sum(int *array, int array_size, int threads_num) {
  if (threads_num <= 0 || array_size == 0) {
    return 0;
  }
  
  if (threads_num > array_size) {
    threads_num = array_size;
  }
  
  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];
  
  int chunk_size = array_size / threads_num;
  int remainder = array_size % threads_num;
  
  int current_start = 0;
  for (int i = 0; i < threads_num; i++) {
    int current_end = current_start + chunk_size;
    
    if (i < remainder) {
      current_end++;
    }
    
    args[i].array = array;
    args[i].begin = current_start;
    args[i].end = current_end;
    
    current_start = current_end;
  }
  
 
  for (int i = 0; i < threads_num; i++) {
    pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]);
  }
  
  int total_sum = 0;
  for (int i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }
  
  return total_sum;
}