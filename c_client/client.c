/*
==============================================================
The Docker Diner - Chef C client
==============================================================
This client connects to Chef C and lets the waiter ask
about points, history, rank, time, or award extra points.
==============================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CHEF_C_STATION "c_server"   // Chef C's station name on the network
#define CHEF_C_PORT    5001

void print_menu(const char *chef_name) {
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║  🍽️  THE DOCKER DINER - Waiter for %s  ║\n", chef_name);
    printf("╚════════════════════════════════════════════╝\n");
    printf("  Pick a question to ask %s:\n\n", chef_name);
    printf("  1) 🍳 Chef, how many dishes have you cooked so far?\n");
    printf("  2) 🧾 Can you show me the updates for the last 5 dishes?\n");
    printf("  3) 🏆 Where do you currently stand on the Top Chef Board?\n");
    printf("  4) ⏰ What time did the last dish go out?\n");
    printf("  5) 🎁 Waiter's treat! I'd like to award extra dish points to this chef.\n\n");
    printf("  Type 'exit' to leave the counter\n");
    printf("========================================\n\n");
}

/* ── Walk up to Chef C's counter ──────────────────────────────── */
int approach_counter() {
    int sock;
    struct sockaddr_in addr;

    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(CHEF_C_PORT);

        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(CHEF_C_STATION, NULL, &hints, &res) == 0) {
            struct sockaddr_in *ip = (struct sockaddr_in *)res->ai_addr;
            addr.sin_addr = ip->sin_addr;
            freeaddrinfo(res);
        }

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            printf("✨ [Waiter] Reached Chef C's counter (%s:%d)\n", CHEF_C_STATION, CHEF_C_PORT);
            return sock;
        }

        printf("⏳ [Waiter] Chef C's counter not open yet, waiting 3s...\n");
        close(sock);
        sleep(3);
    }
}

int main() {
    int sock = approach_counter();
    char question[256], answer[1024], chef_name[128];

    send(sock, "GET_NAME\n", 9, 0);
    memset(chef_name, 0, sizeof(chef_name));
    recv(sock, chef_name, sizeof(chef_name) - 1, 0);
    chef_name[strcspn(chef_name, "\r\n")] = 0;
    if (strlen(chef_name) == 0) {
        strcpy(chef_name, "Chef");
    }

    print_menu(chef_name);

    while (1) {
        printf("🧑‍🍳 Ask %s (Type 'exit' to leave the counter): ", chef_name);
        fflush(stdout);

        if (!fgets(question, sizeof(question), stdin)) break;
        question[strcspn(question, "\n")] = 0;

        if (strcmp(question, "exit") == 0) {
            printf("👋 Waiter leaves the counter. Goodbye!\n");
            break;
        }
        if (strlen(question) == 0) continue;

        if (strcmp(question, "5") == 0) {
            char points_input[64];
            printf("🎁 How many extra dish points to add? ");
            fflush(stdout);
            if (!fgets(points_input, sizeof(points_input), stdin)) break;
            points_input[strcspn(points_input, "\n")] = 0;
            int extra_points = atoi(points_input);
            if (extra_points <= 0) {
                printf("⚠️ Please enter a positive number.\n");
                continue;
            }
            snprintf(question, sizeof(question), "ADD_POINTS %d", extra_points);
        }

        strcat(question, "\n");
        send(sock, question, strlen(question), 0);

        memset(answer, 0, sizeof(answer));
        recv(sock, answer, sizeof(answer) - 1, 0);
        printf("\n[%s says]\n%s\n", chef_name, answer);
    }

    close(sock);
    return 0;
}
