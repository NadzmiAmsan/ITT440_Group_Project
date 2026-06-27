/*
=================================================================
 THE DOCKER DINER - Waiter for Chef C (CLIENT)
=================================================================
 ELI5: This Waiter can now ask Chef C FOUR different questions:

   GET_POINTS  -> "How many dishes have you cooked?"
   GET_HISTORY -> "Show me your last 5 dish updates."
   GET_RANK    -> "What's your rank on the Top Chef Board?"
   GET_TIME    -> "When did you cook your last dish?"

 The Waiter ONLY talks to Chef C - never to the other chefs.
=================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CHEF_C_STATION "c_server"   // Chef C's station name on the network
#define CHEF_C_PORT    5001

void print_menu() {
    printf("\n========================================\n");
    printf("  THE DOCKER DINER - Waiter for Chef C\n");
    printf("========================================\n");
    printf("  Pick a question to ask Chef C:\n\n");
    printf("  1) GET_POINTS  - How many dishes cooked?\n");
    printf("  2) GET_HISTORY - Last 5 dish updates\n");
    printf("  3) GET_RANK    - My rank on Top Chef Board\n");
    printf("  4) GET_TIME    - When was the last dish?\n\n");
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
            printf("[Waiter] Reached Chef C's counter (%s:%d)\n", CHEF_C_STATION, CHEF_C_PORT);
            return sock;
        }

        printf("[Waiter] Chef C's counter not open yet, waiting 3s...\n");
        close(sock);
        sleep(3);
    }
}

int main() {
    int sock = approach_counter();
    char question[256], answer[1024];

    print_menu();

    while (1) {
        printf("Ask Chef C: ");
        fflush(stdout);

        if (!fgets(question, sizeof(question), stdin)) break;
        question[strcspn(question, "\n")] = 0;

        if (strcmp(question, "exit") == 0) {
            printf("Waiter leaves the counter. Goodbye!\n");
            break;
        }
        if (strlen(question) == 0) continue;

        strcat(question, "\n");
        send(sock, question, strlen(question), 0);

        memset(answer, 0, sizeof(answer));
        recv(sock, answer, sizeof(answer) - 1, 0);
        printf("\n[Chef C says]\n%s\n", answer);
    }

    close(sock);
    return 0;
}
