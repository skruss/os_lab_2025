#!/bin/bash

# Очистка предыдущих сборок
rm -f *.o *.a *.so program_static program_dynamic

echo "---Компиляция объектных файлов"
gcc -c -fPIC revert_string.c -o revert_string.o
gcc -c revert_string.c -o revert_string_static.o

echo "---Создание статической библиотеки"
ar rcs librevert_string.a revert_string_static.o

echo "---Создание динамической библиотеки"
gcc -shared -o librevert_string.so revert_string.o

echo "---Компиляция программ"
gcc -o program_static main.c librevert_string.a
gcc -o program_dynamic main.c -L. -lrevert_string -Wl,-rpath='$ORIGIN'

echo "---Проверка зависимостей"
echo "Статическая программа:"
file program_static
echo -e "\nДинамическая программа:"
file program_dynamic
ldd program_dynamic

echo -e "\nСборка завершена"