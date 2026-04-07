#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <libpq-fe.h>

char *get_env_or_default(const char *name, const char *def) {
    char *val = getenv(name);
    return val ? val : (char *)def;
}

char *escape_markdown_v2(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *dest = malloc(len * 2 + 1);
    char *p = dest;
    const char *special = "_*[]()~`>#+-=|{}.!";
    
    while (*src) {
        if (strchr(special, *src)) {
            *p++ = '\\';
        }
        *p++ = *src++;
    }
    *p = '\0';
    return dest;
}

void send_telegram_message(const char *token, const char *chat_id, const char *thread_id, const char *message) {
    CURL *curl;
    CURLcode res;
    
    curl = curl_easy_init();
    if (curl) {
        char url[1024];
        snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage", token);
        
        curl_mime *mime = curl_mime_init(curl);
        curl_mimepart *part;

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "chat_id");
        curl_mime_data(part, chat_id, CURL_ZERO_TERMINATED);

        if (thread_id && strlen(thread_id) > 0) {
            part = curl_mime_addpart(mime);
            curl_mime_name(part, "message_thread_id");
            curl_mime_data(part, thread_id, CURL_ZERO_TERMINATED);
        }

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "text");
        curl_mime_data(part, message, CURL_ZERO_TERMINATED);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "parse_mode");
        curl_mime_data(part, "MarkdownV2", CURL_ZERO_TERMINATED);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        
        // Use SOCKS5 proxy to bypass blocks (via vpn-server tunnel)
        curl_easy_setopt(curl, CURLOPT_PROXY, "socks5h://127.0.0.1:1080");
        
        // Add timeouts to prevent hanging
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
        
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("Message sent successfully.\n");
        }
        
        curl_mime_free(mime);
        curl_easy_cleanup(curl);
    }
}

int main(int argc, char *argv[]) {
    curl_global_init(CURL_GLOBAL_ALL);

    const char *token = get_env_or_default("BOT_TOKEN", "");
    const char *chat_id = get_env_or_default("CHAT_ID", ""); 
    const char *db_conn_str = get_env_or_default("DB_CONN", "user=postgres dbname=lan_install");
    const char *thread_id = getenv("MESSAGE_THREAD_ID"); // Default to NULL/empty for #General

    if (!token || strlen(token) == 0) {
        fprintf(stderr, "Error: BOT_TOKEN environment variable not set.\n");
        curl_global_cleanup();
        return 1;
    }

    if (!chat_id || strlen(chat_id) == 0) {
        fprintf(stderr, "Error: CHAT_ID environment variable not set.\n");
        curl_global_cleanup();
        return 1;
    }

    PGconn *conn = PQconnectdb(db_conn_str);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        curl_global_cleanup();
        return 1;
    }

    const char *query = "SELECT fio, user_id FROM employees "
                        "WHERE is_deleted = false AND is_blocked = false "
                        "AND TO_CHAR(birth_date, 'MM-DD') = TO_CHAR(CURRENT_DATE, 'MM-DD')";
    
    if (argc > 1 && (strcmp(argv[1], "test") == 0 || (argc > 2 && strcmp(argv[2], "test") == 0))) {
        query = "SELECT fio, user_id FROM employees WHERE is_deleted = false AND is_blocked = false AND birth_date IS NOT NULL LIMIT 1";
        printf("Running in TEST mode...\n");
    }

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        curl_global_cleanup();
        return 1;
    }

    int rows = PQntuples(res);
    if (rows == 0) {
        printf("No birthdays today.\n");
    }

    int dry_run = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "dry") == 0) dry_run = 1;
    }

    srand(time(NULL));

    for (int i = 0; i < rows; i++) {
        char *fio = PQgetvalue(res, i, 0);
        char *user_id_str = PQgetvalue(res, i, 1);
        
        char mention[512];
        char *esc_fio = escape_markdown_v2(fio);
        
        if (user_id_str && strlen(user_id_str) > 0) {
            snprintf(mention, sizeof(mention), "[%s](tg://user?id=%s)", esc_fio, user_id_str);
        } else {
            snprintf(mention, sizeof(mention), "%s", esc_fio);
        }

        char first_name[255];
        strncpy(first_name, fio, sizeof(first_name));
        first_name[sizeof(first_name)-1] = '\0';
        char *space = strchr(first_name, ' ');
        if (space) *space = '\0';
        char *esc_first_name = escape_markdown_v2(first_name);

        char message[2048];
        int variant = rand() % 3;
        
        if (variant == 0) {
            snprintf(message, sizeof(message), 
                "Сегодня отличный повод для праздника\\! 🎉 У нашего коллеги %s день рождения\\! 🎂\n\n"
                "%s, поздравляем тебя всей командой\\! Желаем крутых достижений, профессионального роста и отличного настроения\\. Пусть всё задуманное сбывается\\! 🚀",
                mention, esc_first_name);
        } else if (variant == 1) {
            snprintf(message, sizeof(message),
                "Ура\\! Сегодня день рождения у %s\\! 🎈\n\n"
                "Желаем неиссякаемой энергии, побольше ярких моментов и только успешных кейсов\\. Пусть каждый день приносит радость и новые возможности\\! С праздником\\! ✨",
                mention);
        } else {
            snprintf(message, sizeof(message),
                "Друзья, внимание\\! Сегодня празднует свой день рождения %s\\! 🎁\n\n"
                "Давайте поздравим коллегу и накидаем реакций\\! 👇\n"
                "Желаем счастья, вдохновения и реализации самых смелых идей\\. С днем рождения\\!",
                mention);
        }

        if (dry_run) {
            printf("\n[DRY RUN] Сообщение для %s:\n%s\n", fio, message);
        } else {
            printf("Sending greeting for %s...\n", fio);
            send_telegram_message(token, chat_id, thread_id, message);
        }
        
        free(esc_fio);
        free(esc_first_name);
    }

    PQclear(res);
    PQfinish(conn);
    curl_global_cleanup();
    return 0;
}
