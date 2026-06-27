/*
=================================================================
 THE DOCKER DINER - Chef C's Kitchen Station (SERVER)
=================================================================
 ELI5: This is Chef C. Every 30 seconds, Chef C cooks one dish
 and writes it down in the Order Book (database): "+1 dish!"

 A Waiter (client) can now ask Chef C FOUR different questions:

   GET_POINTS  -> "How many dishes have you cooked?"
   GET_HISTORY -> "Show me your last 5 dish updates."
   GET_RANK    -> "What's your rank on the Top Chef Board?"
   GET_TIME    -> "When did you cook your last dish?"

 Chef C works inside their OWN sealed kitchen station
 (Docker container) - completely separate from the other chefs.
=================================================================
*/

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
#define CHEF_NAME   "c_server_user"   // Chef C's name in the Order Book

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

    // "UPSERT": if Chef C's row exists, +1 dish. If not, create it.
    snprintf(query, sizeof(query),
        "INSERT INTO socket_data (user, points, datetime_stamp) "
        "VALUES ('%s', 1, NOW()) "
        "ON DUPLICATE KEY UPDATE points = points + 1, datetime_stamp = NOW()",
        CHEF_NAME);

    if (mysql_query(conn, query))
        fprintf(stderr, "[Order Book ERROR] %s\n", mysql_error(conn));
    else
        printf("[Chef C] 🍳 Dish cooked! Logged in Order Book.\n");

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

/* ── Command 1: GET_POINTS - "How many dishes cooked?" ────────── */
void handle_get_points(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT user, points, datetime_stamp FROM socket_data WHERE user='%s'",
        CHEF_NAME);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row  = mysql_fetch_row(res);
        if (row)
            snprintf(result, result_size,
                "Chef: %s | Dishes Cooked: %s | Last Dish: %s", row[0], row[1], row[2]);
        else
            snprintf(result, result_size, "No record found.");
        mysql_free_result(res);
    }
    mysql_close(conn);
}

/* ── Command 2: GET_HISTORY - "Last 5 dish updates" ────────────── */
void handle_get_history(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT points_before, points_after, updated_at "
        "FROM socket_data_history WHERE user='%s' "
        "ORDER BY updated_at DESC LIMIT 5",
        CHEF_NAME);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
        mysql_close(conn);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (mysql_num_rows(res) == 0) {
        snprintf(result, result_size, "Kitchen Activity Log is empty so far.");
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }

    char line[128];
    strcpy(result, "Kitchen Activity Log (last 5 dishes):\n");
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        snprintf(line, sizeof(line), "  %s -> %s dishes @ %s\n", row[0], row[1], row[2]);
        strncat(result, line, result_size - strlen(result) - 1);
    }
    mysql_free_result(res);
    mysql_close(conn);
}

/* ── Command 3: GET_RANK - "Rank on Top Chef Board" ────────────── */
void handle_get_rank(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT `rank`, points FROM leaderboard WHERE user='%s'",
        CHEF_NAME);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row  = mysql_fetch_row(res);
        if (row)
            snprintf(result, result_size,
                "Chef C's Rank: #%s on the Top Chef Board (with %s dishes)", row[0], row[1]);
        else
            snprintf(result, result_size, "Not ranked yet - no dishes cooked.");
        mysql_free_result(res);
    }
    mysql_close(conn);
}

/* ── Command 4: GET_TIME - "When was the last dish?" ───────────── */
void handle_get_time(char *result, size_t result_size) {
    MYSQL *conn = db_connect();
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT datetime_stamp FROM socket_data WHERE user='%s'",
        CHEF_NAME);

    if (mysql_query(conn, query)) {
        snprintf(result, result_size, "Order Book Error: %s", mysql_error(conn));
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row  = mysql_fetch_row(res);
        if (row)
            snprintf(result, result_size, "Chef C's last dish was cooked at: %s", row[0]);
        else
            snprintf(result, result_size, "No record found.");
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

        if (strcmp(buffer, "GET_POINTS") == 0) {
            handle_get_points(response, sizeof(response));
        } else if (strcmp(buffer, "GET_HISTORY") == 0) {
            handle_get_history(response, sizeof(response));
        } else if (strcmp(buffer, "GET_RANK") == 0) {
            handle_get_rank(response, sizeof(response));
        } else if (strcmp(buffer, "GET_TIME") == 0) {
            handle_get_time(response, sizeof(response));
        } else {
            snprintf(response, sizeof(response),
                "Chef C doesn't understand. Try: GET_POINTS, GET_HISTORY, GET_RANK, GET_TIME");
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
    printf("[Chef C] 👨‍🍳 Kitchen station open on port %d. Ready to cook!\n", PORT);
    printf("[Chef C] Accepted commands: GET_POINTS, GET_HISTORY, GET_RANK, GET_TIME\n");

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
