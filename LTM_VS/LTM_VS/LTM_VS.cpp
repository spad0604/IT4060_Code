#define _WIN32_WINNT 0x0600 
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    WSADATA wsaData;
    WORD wVersion = MAKEWORD(2, 2);

    if (WSAStartup(wVersion, &wsaData)) {
        cout << "Version is not supported" << endl;
        return 1;
    }

    SOCKET server;
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server == INVALID_SOCKET) {
        cout << "Error! Cannot create socket" << endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5500);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (bind(server, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Error! Cannot bind this address." << '\n';
        closesocket(server);
        WSACleanup();
        return 1;
    }

    cout << "Server started" << '\n';

    sockaddr_in clientAddr;
    char buff[1024], clientIP[INET_ADDRSTRLEN];
    int ret, clientPort;
    int clientAddrLen = sizeof(clientAddr);

    while (true) {
        ret = recvfrom(server, buff, sizeof(buff) - 1, 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (ret == SOCKET_ERROR) {
            cout << "Error :" << WSAGetLastError() << '\n';
            continue;
        }

        buff[ret] = '\0';
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
        clientPort = ntohs(clientAddr.sin_port);

        cout << "Received from " << clientIP << ":" << clientPort << " -> " << buff << ': ';

        struct addrinfo hints, *res, *p;
        char ipStr[INET6_ADDRSTRLEN];
        ZeroMemory(&hints, sizeof(hints));
       
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (getaddrinfo(buff, NULL, &hints, &res) != 0) {
            cout << "Failed to resolve domain: " << buff << '\n';
        }
        else {
            cout << "Resolved IP addresses for " << buff << '\n';

            for (p = res; p != NULL; p = p->ai_next) {
                void* addr;
                if (p->ai_family == AF_INET) { //IPv4
                    struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
                    addr = &(ipv4->sin_addr);
                }
                else { //IPv6
                    struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
                    addr = &(ipv6->sin6_addr);
                }

                inet_ntop(p->ai_family, addr, ipStr, sizeof(ipStr));
                std::cout << ipStr << "\n";
            }
        }

        sendto(server, buff, ret, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
        if (res != NULL) {
            freeaddrinfo(res);
        }
    }

    closesocket(server);
    WSACleanup();
    return 0;
}
