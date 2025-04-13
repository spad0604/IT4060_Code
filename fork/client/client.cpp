#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<iostream>
#include<fstream>
#include<signal.h>

const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 5000;

void broadcastName(const char* name) {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(udpSocket < 0) {
        std::cout << "UDP socket creation failed" << std::endl;
        exit(1);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    //Gửi tên của client đến server
    sendto(udpSocket, name, strlen(name), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    close(udpSocket);
}

void requestList() {
    while(true) {
        int tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(tcpSocket < 0) {
            std::cout << "TCP socket creation failed" << std::endl;
            exit(1);
        }

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

        if(connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cout << "Connection to server failed" << std::endl;
            close(tcpSocket);
            exit(1);
        }
        
        //Yêu cầu danh sách client từ server
        char buffer[1024];
        ssize_t bytesRead = recv(tcpSocket, buffer, sizeof(buffer) - 1, 0);
        if(bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::cout << "Danh sách client:" << std::endl;
            std::cout << buffer << std::endl;
        }   
        close(tcpSocket);
        sleep(5);
    }
}

void sigHandler(int sig) {
    std::cout << "Nhận được tín hiệu " << sig << ", thoát..." << std::endl;
    exit(0);
}

int main() {
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    //Tạo tiến trình con để gửi tên và yêu cầu danh sách
    char name[1024];
    std::cout << "Nhập tên của bạn: ";
    std::cin.getline(name, sizeof(name));

    pid_t udpPid = fork();
    if(udpPid == 0) {
        broadcastName(name);
        exit(0);
    }

    //Tạo tiến trình con để yêu cầu danh sách
    pid_t tcpPid = fork();
    if(tcpPid == 0) {
        requestList();
        exit(0);
    }

    //Chờ các tiến trình con kết thúc
    waitpid(udpPid, NULL, 0);
    waitpid(tcpPid, NULL, 0);
    
    return 0;
}
