#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <string.h>

using namespace std;

#define PORT 5500
#define IP_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 1024

#pragma comment(lib,"ws2_32.lib")

int main() {
    // Khởi tạo thư viện Winsock 2.2
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData) != 0) {
        cout << "This version of windows is not supported" << '\n';
        return 0;
    }
    
    // Tạo socket UDP
    SOCKET clientSocket;
    clientSocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(clientSocket == INVALID_SOCKET) {
        cout << "Error! Cannot create socket" << '\n';
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    // Tạo địa chỉ server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    // Tạo buffer để nhận tin nhắn từ server
    char buffer[BUFFER_SIZE];

    cout << "Enter your name: ";
    string name;
    getline(cin,name);

    int serverAddrLen = sizeof(serverAddr);

    // Vòng lặp để gửi và nhận tin nhắn
    while(true) {
        cout << "Enter message: ";
        string message;
        getline(cin,message);

        string fullMessage = name + ": " + message;
        sendto(clientSocket,fullMessage.c_str(),fullMessage.size(),0,(sockaddr*)&serverAddr,serverAddrLen);
        
        int ret = recvfrom(clientSocket,buffer,BUFFER_SIZE,0,(sockaddr*)&serverAddr,&serverAddrLen);
        if(ret == SOCKET_ERROR) {
            cout << "Error! Cannot receive message" << '\n';
            continue;
        }

        buffer[ret] = '\0';
        cout << "Server: " << buffer << '\n';
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
