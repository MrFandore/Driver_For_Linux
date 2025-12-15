#!/bin/bash

echo "=== Тестирование драйвера MYDRIVER ==="

# 1. Сборка
echo "1. Сборка драйвера..."
make clean
make

if [ $? -ne 0 ]; then
    echo "Ошибка сборки!"
    exit 1
fi

echo "✓ Драйвер собран"

# 2. Загрузка с параметрами
echo "2. Загрузка драйвера с параметрами..."
sudo rmmod mydriver 2>/dev/null
sudo insmod mydriver.ko debug=1 timeout_ms=3000 init_message="Тестовое сообщение от драйвера"

if [ $? -ne 0 ]; then
    echo "✗ Ошибка загрузки драйвера"
    exit 1
fi

echo "✓ Драйвер загружен"

# 3. Проверка загрузки
echo "3. Проверка загрузки модуля..."
if lsmod | grep -q mydriver; then
    echo "✓ Модуль загружен в ядро"
else
    echo "✗ Модуль не загружен"
    exit 1
fi

# 4. Проверка устройства
echo "4. Проверка устройства /dev/mydriver..."
if [ -c /dev/mydriver ]; then
    echo "✓ Устройство создано"
else
    echo "✗ Устройство не создано"
    exit 1
fi

# 5. Проверка прав
echo "5. Установка прав доступа..."
sudo chmod 666 /dev/mydriver
echo "✓ Права установлены"

# 6. Тест чтения
echo "6. Тест чтения из устройства..."
echo "Содержимое устройства:"
sudo cat /dev/mydriver
echo ""

# 7. Тест записи
echo "7. Тест записи в устройство..."
echo "Новая строка" | sudo tee /dev/mydriver > /dev/null
echo "Записанная строка:"
sudo cat /dev/mydriver
echo ""

# 8. Компиляция тестовой программы
echo "8. Компиляция тестовой программы..."
gcc -o test_mydriver test_mydriver.c -Wall
if [ $? -ne 0 ]; then
    echo "✗ Ошибка компиляции тестовой программы"
    exit 1
fi
echo "✓ Тестовая программа скомпилирована"

# 9. Запуск тестовой программы
echo "9. Запуск тестовой программы..."
echo "Запустите ./test_mydriver для интерактивного тестирования"
echo ""

# 10. Просмотр логов
echo "10. Логи ядра (последние 20 строк):"
sudo dmesg | tail -20

echo ""
echo "=== Тестирование завершено ==="
echo "Команды для управления:"
echo "  sudo rmmod mydriver    - выгрузить драйвер"
echo "  sudo dmesg | tail -20  - просмотреть логи"
echo "  ./test_mydriver        - запустить тестовую программу"