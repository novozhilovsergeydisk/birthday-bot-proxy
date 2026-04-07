# DrLanInstall Bot — Context Documentation

## Project Overview

**birthday_bot** — это Telegram-бот на C, который автоматически поздравляет сотрудников с днём рождения. Бот ежедневно проверяет базу данных PostgreSQL на наличие сотрудников, у которых сегодня день рождения, и отправляет персонализированные поздравления в Telegram-группу (форум).

### Основные компоненты

| Файл | Описание |
|------|----------|
| `birthday_bot.c` | Исходный код бота на C |
| `Makefile` | Сборка проекта через `make` |
| `deploy.sh` | Скрипт деплоя на удалённый сервер |
| `birthday_bot/` | Директория (возможно, для данных или логов) |

### Технологии

- **Язык:** C (GCC)
- **Библиотеки:** `libpq` (PostgreSQL), `libcurl` (Telegram API)
- **Сервер:** `lan-install` (удалённый хост для деплоя)
- **Планировщик:** cron (ежедневный запуск в 09:00)

## Building and Running

### Сборка (локально)

```bash
make          # компиляция
make clean    # очистка
```

### Переменные окружения

| Переменная | Описание | Значение по умолчанию |
|------------|----------|-----------------------|
| `BOT_TOKEN` | Токен Telegram-бота | `8629292027:AAGXg5oSinbYU1DYXnF5TFewg1fcXsBFox4` |
| `CHAT_ID` | ID чата (группа/форум) | `-1003689380574` |
| `DB_CONN` | Строка подключения к PostgreSQL | `user=postgres dbname=lan_install` |
| `MESSAGE_THREAD_ID` | ID топика в форуме (опционально) | — |

### Запуск

```bash
# Обычный режим (проверка сегодняшних именинников)
BOT_TOKEN="..." CHAT_ID="..." DB_CONN="..." ./birthday_bot

# Тестовый режим (один случайный сотрудник)
./birthday_bot test
```

## Deployment

### Команда деплоя

Использовать **только** команду `./deploy "комментарий"` — она сама делает git add, git commit и git push.

```bash
./deploy "Добавил новую функциональность"
```

### Что делает deploy.sh

1. Создаёт директорию на сервере `lan-install:~/birthday_bot`
2. Синхронизирует файлы через `rsync` (исключая `.git` и бинарник)
3. Компилирует на сервере (`make clean && make`)
4. Добавляет задачу в crontab (запуск ежедневно в 09:00)

### Cron-задача

```
0 9 * * * cd ~/birthday_bot && sudo -u postgres BOT_TOKEN="..." CHAT_ID="..." MESSAGE_THREAD_ID="..." ./birthday_bot >> birthday_bot.log 2>&1
```

## Database Schema

Бот ожидает таблицу `employees` со следующей структурой:

```sql
CREATE TABLE employees (
    fio VARCHAR,          -- ФИО сотрудника
    user_id VARCHAR,      -- Telegram user ID для упоминаний
    birth_date DATE,      -- Дата рождения
    is_deleted BOOLEAN,   -- Флаг удаления
    is_blocked BOOLEAN    -- Флаг блокировки
);
```

## Features

- **MarkdownV2** форматирование поздравлений с экранированием спецсимволов
- **3 варианта** текста поздравления (случайный выбор)
- **Поддержка форумов** Telegram (через `message_thread_id`)
- **Упоминания** сотрудников через `tg://user?id=...`

## Development Notes

- Флаг компиляции `-Wall -O2`
- Требуется `pkg-config` для `libpq` и `libcurl`
- Логирование в `birthday_bot.log` на сервере
