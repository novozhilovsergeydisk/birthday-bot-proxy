#!/bin/bash

# Configuration
SERVER="lan-install"
REMOTE_DIR="~/birthday_bot"

# Load environment variables if .env exists locally
if [ -f .env ]; then
    export $(grep -v '^#' .env | xargs)
fi

if [[ -z "$BOT_TOKEN" || -z "$CHAT_ID" ]]; then
    echo "ERROR: BOT_TOKEN and CHAT_ID must be set (locally or in .env)"
    exit 1
fi

echo "Deploying to $SERVER..."

# Create remote directory
ssh $SERVER "mkdir -p $REMOTE_DIR"

# Sync files
rsync -avz --exclude '.git' --exclude 'birthday_bot' . $SERVER:$REMOTE_DIR

# Compile on server
ssh $SERVER "cd $REMOTE_DIR && make clean && make && chmod +x run_bot.sh"

# Add to crontab if not already there (runs daily at 9:00 AM)
# Variables are now loaded from .env inside run_bot.sh
CRON_JOB="0 9 * * * cd $REMOTE_DIR && ./run_bot.sh >> birthday_bot.log 2>&1"
ssh $SERVER "(crontab -l 2>/dev/null | grep -v 'birthday_bot'; echo \"$CRON_JOB\") | crontab -"

echo "Deployment complete. Bot with proxy auto-repair scheduled daily at 09:00."
