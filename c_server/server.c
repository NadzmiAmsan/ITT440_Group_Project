#define _GNU_SOURCE   /* needed for timegm() and strptime() on glibc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <mysql.h>
#include <time.h>

#define PORT        5001          // Chef C's station "door number"
#define DB_HOST     "database"    // The Diner's order book location
#define DB_USER     "itt440_user"
#define DB_PASS     "itt440_pass"
#define DB_NAME     "itt440_db"

#define KL_OFFSET_SECONDS (8 * 3600)  // GMT+8 Kuala Lumpur

/* ── Convert a MySQL "YYYY-MM-DD HH:MM:SS" UTC string into a    ──
   ── GMT+8 (Asia/Kuala_Lumpur) formatted string.                ── */
void to_kl_time(const char *utc_str, char *out, size_t out_size) {
    if (utc_str == NULL) {
        snprintf(out, out_size, "N/A");
        return;
    }

    struct tm tm_val;
    memset(&tm_val, 0, sizeof(tm_val));

    if (strptime(utc_str, "%Y-%m-%d %H:%M:%S", &tm_val) == NULL) {
        // Couldn't parse, just echo the original value back
        snprintf(out, out_size, "%s", utc_str);
        return;
    }

    // Interpret the parsed fields as UTC (NOT local time)
    time_t utc_time = timegm(&tm_val);
    time_t kl_time   = utc_time + KL_OFFSET_SECONDS;

    struct tm kl_tm;
    gmtime_r(&kl_time, &kl_tm);

    strftime(out, out_size, "%d-%m-%Y %H:%M:%S", &kl_tm);
}

const char *get_configured_chef_name(void) {
    const char *chef_name = getenv("CHEF_NAME");
    if (chef_name != NULL && chef_name[0] != '\0')
        return chef_name;
    return "Chef";
}

/* ── Connect to the Order Book (Database) ────────────────────── */
MYSQL *db_connect() {
    MYSQL *conn;
    while (1) {
        conn = mysql_init(NULL);
        if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 3306, NULL, 0))
            return conn;
        fprintf(stderr, "[Order Book] Not ready, waiting 3s...\n");
        mysql_close(conn);
        sleep(3);
    }
}

/* ── Chef C cooks one dish and logs it ────────────────────────── */
void cook_one_dish() {
    MYSQL *conn = db_connect();
    char query[512];
    const char *chef_name = get_configured_chef_name();

    // "UPSERT": if the chef row exists, +1 dish. If not, create it.
    snprintf(query, sizeof(query),
        "INSERT INTO socket_data (user, points, datetime_stamp) "
        "VALUES ('%s', 1, NOW()) "
        "ON DUPLICATE KEY UPDATE points = points + 1, datetime_stamp = NOW()",
        chef_name);

    if (mysql_query(conn, query))
        fprintf(stderr, "[Order Book ERROR] %s\n", mysql_error(conn));
    else
        printf("[%s] 🍳 Dish cooked! Logged in Order Book.\n", chef_name);

    mysql_close(conn);
}

/* ── Background thread: cook a dish every 30 seconds ──────────── */
void *kitchen_timer(void *arg) {
    while (1) {
        cook_one_dish();
        sleep(30);   // Chef C cooks once every 30 seconds
    }
    return NULL;
}

/* ── Helper: GET_NAME - ask the current chef name from the database ─── */
void handle_get_name(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    const char *chef_name = get_configured_chef_name();
    snprintf(query, sizeof(query),
        "SELECT user FROM socket_data WHERE user='%s'",
        chef_name);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "%s", chef_name);
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row  = mysql_fetch_row(res);
        if (row && row[0])
            snprintf(result, result_size, "%s", row[0]);
        else
            snprintf(result, result_size, "%s", chef_name);
        mysql_free_result(res);
    }
    mysql_close(conn);
}

/* ── Command 5: ADD_POINTS - add extra dishes to the current chef ─── */
void handle_add_points(char *result, size_t result_size, const char *request) {
    MYSQL *conn = db_connect();
    const char *chef_name = get_configured_chef_name();
    int amount = 0;

    if (sscanf(request, "ADD_POINTS %d", &amount) != 1 || amount <= 0) {
        snprintf(result, result_size, "Please use: ADD_POINTS <number>");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query),
        "INSERT INTO socket_data (user, points, datetime_stamp) "
        "VALUES ('%s', 0, NOW()) "
        "ON DUPLICATE KEY UPDATE points = points + %d, datetime_stamp = NOW()",
        chef_name, amount);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
    } else {
        snprintf(result, result_size, "Added %d extra dish point(s) to %s.", amount, chef_name);
    }

    mysql_close(conn);
}

/* ── Command 1: GET_POINTS - "How many dishes cooked?" ────────── */
void handle_get_points(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    const char *chef_name = get_configured_chef_name();
    snprintf(query, sizeof(query),
        "SELECT user, points, datetime_stamp FROM socket_data WHERE user='%s'",
        chef_name);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row  = mysql_fetch_row(res);
        if (row) {
            char kl_buf[64];
            to_kl_time(row[2], kl_buf, sizeof(kl_buf));
            snprintf(result, result_size,
                "I have cooked %s dishes so far! My last dish was served at %s (GMT+8 KL).", row[1], kl_buf);
        } else
            snprintf(result, result_size, "I haven't cooked anything yet — no record found.");
        mysql_free_result(res);
    }
    mysql_close(conn);
}

/* ── Command 2: GET_HISTORY - "Last 5 dish updates" ────────────── */
void handle_get_history(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    const char *chef_name = get_configured_chef_name();
    snprintf(query, sizeof(query),
        "SELECT points_before, points_after, updated_at "
        "FROM socket_data_history WHERE user='%s' "
        "ORDER BY updated_at DESC LIMIT 5",
        chef_name);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
        mysql_close(conn);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (mysql_num_rows(res) == 0) {
        snprintf(result, result_size, "I haven't logged any dishes yet — my kitchen activity log is empty!");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }

    char line[160];
    char kl_buf[64];
    strcpy(result, "Here are my last 5 dish updates:\n");
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        to_kl_time(row[2], kl_buf, sizeof(kl_buf));
        snprintf(line, sizeof(line), "  I went from %s to %s dishes @ %s (GMT+8 KL)\n", row[0], row[1], kl_buf);
        strncat(result, line, result_size - strlen(result) - 1);
    }
    mysql_free_result(res);
    mysql_close(conn);
}

/* ── Command 3: GET_RANK - "Rank on Top Chef Board" ────────────── */
void handle_get_rank(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    const char *chef_name = get_configured_chef_name();
    snprintf(query, sizeof(query),
        "SELECT `rank`, points FROM leaderboard WHERE user='%s'",
        chef_name);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row  = mysql_fetch_row(res);
        if (row)
            snprintf(result, result_size,
                "I am currently ranked #%s on the Top Chef Board, with %s dishes cooked!", row[0], row[1]);
        else
            snprintf(result, result_size, "I am not ranked yet — I haven't cooked any dishes.");
        mysql_free_result(res);
    }
    mysql_close(conn);
}

/* ── Command 4: GET_TIME - "When was the last dish?" ───────────── */
void handle_get_time(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    const char *chef_name = get_configured_chef_name();
    snprintf(query, sizeof(query),
        "SELECT datetime_stamp FROM socket_data WHERE user='%s'",
        chef_name);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row  = mysql_fetch_row(res);
        if (row) {
            char kl_buf[64];
            to_kl_time(row[0], kl_buf, sizeof(kl_buf));
            snprintf(result, result_size, "My last dish was cooked at %s (GMT+8 KL).", kl_buf);
        } else
            snprintf(result, result_size, "I haven't cooked any dishes yet — no time to report.");
        mysql_free_result(res);
    }
    mysql_close(conn);
}

/* ── Handle one Waiter (client) asking questions ──────────────── */
void *serve_waiter(void *arg) {
    int waiter_fd = *(int *)arg;
    free(arg);

    char buffer[1024];
    char response[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(waiter_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;

        buffer[strcspn(buffer, "\r\n")] = 0;   // remove newline
        printf("[Waiter asked] %s\n", buffer);

        memset(response, 0, sizeof(response));

        char command[32];
        strcpy(command, buffer);

        if (strncmp(buffer, "ADD_POINTS", 10) == 0) {
            strcpy(command, "ADD_POINTS");
        } else if (strcmp(command, "1") == 0) {
            strcpy(command, "GET_POINTS");
        } else if (strcmp(command, "2") == 0) {
            strcpy(command, "GET_HISTORY");
        } else if (strcmp(command, "3") == 0) {
            strcpy(command, "GET_RANK");
        } else if (strcmp(command, "4") == 0) {
            strcpy(command, "GET_TIME");
        } else if (strcmp(command, "5") == 0) {
            strcpy(command, "ADD_POINTS");
        }

        if (strcmp(command, "GET_NAME") == 0) {
            handle_get_name(response, sizeof(response));
        } else if (strcmp(command, "GET_POINTS") == 0) {
            handle_get_points(response, sizeof(response));
        } else if (strcmp(command, "GET_HISTORY") == 0) {
            handle_get_history(response, sizeof(response));
        } else if (strcmp(command, "GET_RANK") == 0) {
            handle_get_rank(response, sizeof(response));
        } else if (strcmp(command, "GET_TIME") == 0) {
            handle_get_time(response, sizeof(response));
        } else if (strcmp(command, "ADD_POINTS") == 0) {
            handle_add_points(response, sizeof(response), buffer);
        } else {
            snprintf(response, sizeof(response),
                "%s doesn't understand. Try: 1, 2, 3, 4, or 5", get_configured_chef_name());
        }

        strncat(response, "\n", sizeof(response) - strlen(response) - 1);
        send(waiter_fd, response, strlen(response), 0);
    }
    close(waiter_fd);
    printf("[-] Waiter left the counter\n");
    return NULL;
}

/* ── Main: open Chef C's kitchen station ──────────────────────── */
int main() {
    // Start the "cook every 30 seconds" background timer
    pthread_t timer;
    pthread_create(&timer, NULL, kitchen_timer, NULL);
    pthread_detach(timer);

    // Open the kitchen counter (TCP socket) for Waiters to approach
    int counter_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(counter_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    bind(counter_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(counter_fd, 5);
    printf("[%s] 👨‍🍳 Kitchen station open on port %d. Ready to cook!\n", get_configured_chef_name(), PORT);
    printf("[%s] Accepted commands: 1, 2, 3, 4\n", get_configured_chef_name());

    // Keep welcoming Waiters forever
    while (1) {
        struct sockaddr_in waiter_addr;
        socklen_t len = sizeof(waiter_addr);
        int *waiter_fd = malloc(sizeof(int));
        *waiter_fd = accept(counter_fd, (struct sockaddr *)&waiter_addr, &len);
        printf("[+] A Waiter approached the counter: %s\n", inet_ntoa(waiter_addr.sin_addr));

        pthread_t tid;
        pthread_create(&tid, NULL, serve_waiter, waiter_fd);
        pthread_detach(tid);
    }
    return 0;
}