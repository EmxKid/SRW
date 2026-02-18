#!/bin/bash
set -e  # Прервать при первой ошибке

cd "$(dirname "$0")"

echo "Очистка старой сборки..."
rm -rf build

echo "Создание каталога сборки..."
mkdir build && cd build

echo "Генерация Makefile..."
cmake .. || { echo "Ошибка CMake!"; exit 1; }

echo "Компиляция проекта..."
make -j$(nproc) || { echo "Ошибка компиляции!"; exit 1; }

echo "Запуск симулятора..."
./simulator_example