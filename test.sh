#!/bin/bash

# Проверка зависимостей
if ! command -v cmake &> /dev/null; then
    echo "Ошибка: cmake не установлен."
    exit 1
fi

# Очистка предыдущих данных
rm -rf build /tmp/uringkv
mkdir -p /tmp/uringkv

# Сборка
mkdir build && cd build
cmake .. && make
if [ $? -ne 0 ]; then
    echo "Ошибка сборки."
    exit 1
fi
cd ..

# Запуск сервера в фоновом режиме
./build/uringkv --path /tmp/uringkv --queue-depth 256 --segment 128M &
SERVER_PID=$!

# Дать серверу время на старт
sleep 1

# Тесты
echo "Тест 1: PUT и GET"
./build/uringkv put test_key test_value
if [ $? -eq 0 ]; then
    VAL=$(./build/uringkv get test_key)
    if [ "$VAL" = "test_value" ]; then
        echo "Тест 1 прошел: PUT/GET OK"
    else
        echo "Тест 1 провалился: неверное значение ($VAL)"
        kill $SERVER_PID
        exit 1
    fi
else
    echo "Тест 1 провалился: ошибка PUT"
    kill $SERVER_PID
    exit 1
fi

echo "Тест 2: DEL"
./build/uringkv del test_key
VAL=$(./build/uringkv get test_key)
if [ -z "$VAL" ]; then
    echo "Тест 2 прошел: DEL OK"
else
    echo "Тест 2 провалился: значение осталось ($VAL)"
    kill $SERVER_PID
    exit 1
fi

echo "Тест 3: BENCH и метрики"
./build/uringkv bench --keys 1000 --read 70 --write 30 --duration 2s
if [ $? -eq 0 ]; then
    echo "Тест 3 прошел: BENCH OK"
else
    echo "Тест 3 провалился: ошибка BENCH"
    kill $SERVER_PID
    exit 1
fi

echo "Тест 4: Сбой и восстановление"
kill -9 $SERVER_PID
sleep 1
./build/uringkv --path /tmp/uringkv --queue-depth 256 --segment 128M &
NEW_PID=$!
sleep 1
VAL=$(./build/uringkv get test_key)
if [ "$VAL" = "test_value" ]; then
    echo "Тест 4 прошел: восстановление OK"
else
    echo "Тест 4 провалился: данные потеряны ($VAL)"
fi

# Остановка сервера
kill $NEW_PID
rm -rf build /tmp/uringkv

echo "Все тесты завершены."