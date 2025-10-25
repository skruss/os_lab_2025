# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200112L -I.
LDFLAGS = -lpthread

# Целевые исполняемые файлы
TARGETS = parallel_min_max process_memory parallel_sum

# Основная цель - сборка всех программ
all: $(TARGETS)

# Сборка parallel_min_max
parallel_min_max: parallel_min_max.o utils.o find_min_max.o
	$(CC) -o $@ parallel_min_max.o utils.o find_min_max.o $(LDFLAGS)

# Сборка process_memory
process_memory: process_memory.o
	$(CC) -o $@ process_memory.o $(LDFLAGS)

# Сборка parallel_sum
parallel_sum: parallel_sum.o utils.o sum.o
	$(CC) -o $@ parallel_sum.o utils.o sum.o $(LDFLAGS)

# Объектные файлы
parallel_min_max.o: parallel_min_max.c utils.h find_min_max.h
	$(CC) $(CFLAGS) -c parallel_min_max.c -o parallel_min_max.o

process_memory.o: process_memory.c
	$(CC) $(CFLAGS) -c process_memory.c -o process_memory.o

parallel_sum.o: parallel_sum.c utils.h sum.h
	$(CC) $(CFLAGS) -c parallel_sum.c -o parallel_sum.o

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c -o utils.o

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -c find_min_max.c -o find_min_max.o

sum.o: sum.c sum.h
	$(CC) $(CFLAGS) -c sum.c -o sum.o

# Очистка
clean:
	rm -f *.o $(TARGETS)

# Пересборка
rebuild: clean all

# Псевдоцели
.PHONY: all clean rebuild

# Помощь
help:
	@echo "Доступные цели:"
	@echo "  all      - собрать все программы (по умолчанию)"
	@echo "  clean    - удалить все объектные и исполняемые файлы"
	@echo "  rebuild  - пересобрать все программы"
	@echo "  help     - показать эту справку"
	@echo ""
	@echo "Целевые программы:"
	@echo "  parallel_min_max - параллельный поиск min/max в массиве (процессы)"
	@echo "  process_memory   - демонстрация структуры памяти процесса"
	@echo "  parallel_sum     - параллельное вычисление суммы массива (потоки)"