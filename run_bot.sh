#!/bin/bash

# Переход в директорию скрипта
cd "$(dirname "$0")"

# Загрузка переменных из .env, если он существует
if [ -f .env ]; then
    export $(grep -v '^#' .env | xargs)
fi

# Параметры из окружения
TOKEN=${BOT_TOKEN}
ID=${CHAT_ID}
THREAD=${MESSAGE_THREAD_ID}

if [[ -z "$TOKEN" || -z "$ID" ]]; then
    echo "ERROR: BOT_TOKEN or CHAT_ID not set in environment."
    exit 1
fi

PROXY="socks5h://127.0.0.1:1080"
TEST_URL="https://api.telegram.org/bot$TOKEN/getMe"

echo "[$(date)] Starting birthday_bot check..."

# 1. Проверяем работоспособность прокси
check_proxy() {
    curl -s -x "$PROXY" --connect-timeout 5 "$TEST_URL" | grep -q '"ok":true'
    return $?
}

if ! check_proxy; then
    echo "[$(date)] Proxy is down or Telegram is unreachable. Attempting to restart tunnel..."
    
    # Пытаемся корректно убить старый процесс, если порт занят чем-то не тем
    # fuser может быть не установлен, поэтому используем pkill для ssh к vpn-server
    pkill -f "ssh -f -N -D 127.0.0.1:1080 vpn-server" 2>/dev/null
    sleep 1
    
    # Поднимаем туннель от имени текущего пользователя (предполагается root в cron)
    ssh -o BatchMode=yes -o ConnectTimeout=10 -f -N -D 127.0.0.1:1080 vpn-server
    
    # Даем время на установку соединения
    sleep 3
    
    if ! check_proxy; then
        echo "[$(date)] ERROR: Could not restore proxy connection. Exiting."
        exit 1
    fi
    echo "[$(date)] Proxy tunnel restored successfully."
else
    echo "[$(date)] Proxy is working fine."
fi

# 2. Запускаем бота
# Мы заходим в директорию скрипта, чтобы логи писались рядом
cd "$(dirname "$0")"
sudo -u postgres BOT_TOKEN="$TOKEN" CHAT_ID="$ID" MESSAGE_THREAD_ID="$THREAD" ./birthday_bot "$@"
