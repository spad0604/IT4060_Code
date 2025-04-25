#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<pthread.h>

int g_client[1024] = {};
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct in_addr IN_ADDR;

int g_count = 0;

void* ClienThread(void* param) {
    int c = *((int*)param);

    while(1) {
        char buffer[1024] = {0};
        int r = recv(c, buffer, sizeof(buffer - 1), 0);

        if(r >= 0) {
            for(int i = 0;i < g_count;i ++) {
                if(g_client[i] != c)  {
                    send(g_client[i], buffer, strlen(buffer), 0);
                }
            }
        } else {
            break;
        }
    }
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in saddr, caddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if(bind(s, (SOCKADDR*)&saddr, sizeof(saddr))){
        listen(s, 10);
        while(1) {
            socklen_t  clen = sizeof(SOCKADDR_IN);
            int c = accept(s, (SOCKADDR*)&caddr, &clen);

            if(c >= 0) {
                g_client[g_count]++;
                g_count++;
                pthread_t pid;
                int* params = (int*) calloc(1, sizeof(int));
                *params = c;
                pthread_create(&pid, NULL, ClienThread, (void*)params);
            }
        }
    }
}