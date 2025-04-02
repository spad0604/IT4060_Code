#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct in_addr IN_ADDR;
typedef char* STRING;

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed!\n");
        return 1;
    }

    // Create UDP socket
    SOCKET u = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN baddr, saddr;
    int slen = sizeof(saddr);
    char buffer[1024] = {0};

    // Set up broadcast address
    baddr.sin_family = AF_INET;
    baddr.sin_port = htons(9999);
    baddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    // Enable broadcast
    BOOL bOptVal = TRUE;
    setsockopt(u, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, sizeof(BOOL));

    // Find server
    printf("Looking for server...\n");
    const char* query = "WHERE IS THE SERVER?";
    sendto(u, query, strlen(query), 0, (SOCKADDR*)&baddr, sizeof(SOCKADDR));
    
    int r = recvfrom(u, buffer, sizeof(buffer)-1, 0, (SOCKADDR*)&saddr, &slen);
    if (r > 0) {
        buffer[r] = 0;
        printf("Found server at %s", buffer);
        
        // Get file list
        const char* listQuery = "WHAT FILE DO YOU HAVE?";
        sendto(u, listQuery, strlen(listQuery), 0, (SOCKADDR*)&saddr, sizeof(SOCKADDR));
        
        while (1) {
            memset(buffer, 0, sizeof(buffer));
            r = recvfrom(u, buffer, sizeof(buffer)-1, 0, (SOCKADDR*)&saddr, &slen);
            if (r > 0) {
                buffer[r] = 0;
                printf("\nAvailable files:\n%s", buffer);
                
                // Get filename from user
                printf("Enter filename to download (or 'quit' to exit): ");
                char filename[256];
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = 0;

                if (strcmp(filename, "quit") == 0) {
                    break;
                }

                // Request file
                char request[512] = "GET ";
                strcat(request, filename);
                sendto(u, request, strlen(request), 0, (SOCKADDR*)&saddr, sizeof(SOCKADDR));

                memset(buffer, 0, sizeof(buffer));
                r = recvfrom(u, buffer, sizeof(buffer)-1, 0, (SOCKADDR*)&saddr, &slen);
                if (r > 0) {
                    buffer[r] = 0;
                    if (strcmp(buffer, "NOT AVAILABLE") == 0) {
                        printf("File not available\n");
                        continue;  
                    }
                    else if (strcmp(buffer, "READY, PLEASE CONNECT") == 0) {
                        // Connect via TCP
                        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        SOCKADDR_IN tcp_addr;
                        tcp_addr.sin_family = AF_INET;
                        tcp_addr.sin_port = htons(8888);
                        tcp_addr.sin_addr = saddr.sin_addr;

                        if (connect(s, (SOCKADDR*)&tcp_addr, sizeof(SOCKADDR)) == 0) {
                            // Receive file size
                            int filesize;
                            recv(s, (char*)&filesize, 4, 0);

                            // Create download directory if it doesn't exist
                            _mkdir("D:\\download");

                            // Create full path for downloaded file
                            char fullPath[512];
                            sprintf(fullPath, "D:\\download\\%s", filename);

                            // Open file for writing
                            FILE* f = fopen(fullPath, "wb");
                            if (f != NULL) {
                                int received = 0;
                                while (received < filesize) {
                                    r = recv(s, buffer, sizeof(buffer), 0);
                                    if (r > 0) {
                                        fwrite(buffer, 1, r, f);
                                        received += r;
                                    }
                                }
                                fclose(f);
                                printf("File downloaded successfully to D:\\download\\%s\n", filename);
                            }
                            closesocket(s);
                        }
                    }
                }
            }
        }
    }

    closesocket(u);
    WSACleanup();
    return 0;
}
