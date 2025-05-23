#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include<WinSock2.h>
#include<iostream>
#include<string.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct in_addr IN_ADDR;
typedef char* STRING;

STRING listFiles[] =
{
    "ltm1.c",
    "ltm2.c",
    "ltm3.c",
    "ltm4.c",
    "ltm5.c",
    "ltm6.c",
    "ltm7.c"
};
int listCount = 7;

char* GetServerIP()
{
    return "172.20.38.136\n";
}

int main()
{
    int u = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN uaddr, caddr;
    int clen;
    uaddr.sin_family = AF_INET;
    uaddr.sin_port = htons(9999);
    uaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    int error = bind(u, (SOCKADDR*)&uaddr, sizeof(SOCKADDR));
    if (error == 0)
    {
        char buffer[1024] = { 0 };
        while (0 == 0)
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

                if (strcmp(buffer, "WHERE IS THE SERVER?") == 0)
                {
                    char* serverIP = GetServerIP();
                    sendto(u, serverIP, strlen(serverIP), 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                }
                if (strcmp(buffer, "WHAT FILE DO YOU HAVE?") == 0)
                {
                    memset(buffer, 0, sizeof(buffer));
                    for (int i = 0; i < listCount; i++)
                    {
                        sprintf(buffer + strlen(buffer), "%s\n", listFiles[i]);
                    }
                    sendto(u, buffer, strlen(buffer), 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                }
                if (strncmp(buffer, "GET ", 4) == 0)
                {
                    char* filename = buffer + 4;
                    while (filename[strlen(filename) - 1] == '\r' || filename[strlen(filename) - 1] == '\n')
                    {
                        filename[strlen(filename) - 1] = 0;
                    }
                    char filepath[1024] = { 0 };
                    sprintf(filepath, "/mnt/g/WSL/%s", filename);
                    FILE* f = fopen(filepath, "rb");
                    if (f == NULL)
                    {
                        sendto(u, "NOT AVAILABLE", 13, 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                    }
                    else
                    {
                        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
                            int c = accept(s, (SOCKADDR*)&tmpAddr, &tmpLen);

                            fseek(f, 0, SEEK_END);
                            int flen = ftell(f);
                            send(c, &flen, 4, 0);
                            fseek(f, 0, SEEK_SET);
                            int sent = 0;
                            while (sent < flen)
                            {
                                int r = fread(buffer, 1, sizeof(buffer), f);
                                send(c, buffer, r, 0);
                                sent += r;
                            }
                            fclose(f);
                            close(c);
                        }
                        else
                        {
                            sendto(u, "NOT AVAILABLE", 13, 0, (SOCKADDR*)&caddr, sizeof(SOCKADDR));
                            fclose(f);
                        }
                        close(s);
                    }
                }
            }
        }
    }
    else
    {
        printf("Failed to bind!\n");
        close(u);
    }
    return 0;
}