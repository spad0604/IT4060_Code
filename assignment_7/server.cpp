#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdlib.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct in_addr IN_ADDR; 
typedef char* STRING;

const char* listFiles[] = 
{
    "test.txt",
    "test2.txt", 
    "test3.txt",
    "test4.txt",
};
int listCount = sizeof(listFiles) / sizeof(listFiles[0]);

const char* GetServerIP()
{
    return "127.0.0.1\n";
}

int main()
{
    // Khởi tạo Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed!\n");
        return 1;
    }

    SOCKET u = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN uaddr, caddr;
    int clen;
    uaddr.sin_family = AF_INET;
    uaddr.sin_port = htons(9999);
    uaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    int error = bind(u, (SOCKADDR*)&uaddr, sizeof(SOCKADDR));
    if (error == 0)
    {
        char buffer[1024] = { 0 };
        while (1)
        {
            memset(buffer, 0, sizeof(buffer));
            clen = sizeof(caddr);
            int r = recvfrom(u, buffer, sizeof(buffer) - 1, 0, (SOCKADDR*)&caddr, &clen);
            if (r > 0)
            {
                while (buffer[strlen(buffer) - 1] == '\r' || buffer[strlen(buffer) - 1] == '\n')
                {
                    buffer[strlen(buffer) - 1] = 0;
                }

                if (strcmp(buffer,"WHERE IS THE SERVER?") == 0)
                {
                    const char* serverIP = GetServerIP();
                    sendto(u, serverIP, strlen(serverIP), 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                }
                if (strcmp(buffer,"WHAT FILE DO YOU HAVE?") == 0)
                {
                    memset(buffer, 0, sizeof(buffer));
                    for (int i = 0;i < listCount;i++)
                    {
                        sprintf(buffer + strlen(buffer), "%s\n", listFiles[i]);
                    }
                    sendto(u, buffer, strlen(buffer), 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                }
                if (strncmp(buffer,"GET ", 4) == 0)
                {
                    char* filename = buffer + 4;
                    while (filename[strlen(filename) - 1] == '\r' || filename[strlen(filename) - 1] == '\n')
                    {
                        filename[strlen(filename) - 1] = 0;
                    }
                    char filepath[1024] = { 0 };
                    sprintf(filepath, "./%s", filename); 
                    FILE* f = fopen(filepath, "rb");
                    if (f == NULL)
                    {
                        sendto(u, "NOT AVAILABLE", 13, 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                    }
                    else
                    {
                        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        SOCKADDR_IN saddr;
                        saddr.sin_family = AF_INET;
                        saddr.sin_port = htons(8888);
                        saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
                        error = bind(s, (SOCKADDR*)&saddr, sizeof(SOCKADDR));
                        if (error == 0)
                        {
                            listen(s, 10);
                            sendto(u, "READY, PLEASE CONNECT", 21, 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                            SOCKADDR_IN tmpAddr;
                            int tmpLen = sizeof(tmpAddr);
                            SOCKET c = accept(s, (SOCKADDR*)&tmpAddr, &tmpLen);

                            fseek(f, 0, SEEK_END);
                            int flen = ftell(f);
                            send(c, (char*)&flen, 4, 0);
                            fseek(f, 0, SEEK_SET);
                            int sent = 0;
                            while (sent < flen)
                            {
                                int r = fread(buffer, 1, sizeof(buffer), f);
                                send(c, buffer, r, 0);
                                sent += r;
                            }
                            fclose(f);
                            closesocket(c);
                        }
                        else
                        {
                            sendto(u, "NOT AVAILABLE", 13, 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                            fclose(f);
                        }
                        closesocket(s);
                    }
                }
            }
        }
    }
    else
    {
        printf("Failed to bind!\n");
        closesocket(u);
    }

    WSACleanup();
    return 0;
}