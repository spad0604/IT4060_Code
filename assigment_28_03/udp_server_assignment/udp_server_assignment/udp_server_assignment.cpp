//Code by Nguyen Van Giap - 20225126
#include <iostream>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>

#define PORT 5500
#define IP_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 1024

using namespace std;

#pragma comment(lib, "ws2_32.lib")

vector<sockaddr_in> clientList;

int main()
{
    // Khởi tạo thư viện Winsock 2.2
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "This version of windows is not supported" << endl;
        return 0;
    }

    // Tạo socket UDP
    SOCKET serverSocket;
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET)
    {
        cout << "Error! Cannot create socket" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

    // Tạo địa chỉ server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET,IP_ADDRESS,&serverAddr.sin_addr);

    // Bind socket với địa chỉ server
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "Error! Cannot bind socket" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

    cout << "Server started at " << IP_ADDRESS << ":" << PORT << endl;

    char buffer[BUFFER_SIZE];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int ret;

    // Lắng nghe và xử lý tin nhắn từ client
    while (true)
    {
        ret = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (ret == SOCKET_ERROR)
        {
            cout << "Error! Cannot receive message" << '\n';
            continue;
        }

        bool isNewClient = true;
        for (auto& client : clientList)
        {
            if (client.sin_addr.s_addr == clientAddr.sin_addr.s_addr &&
                client.sin_port == clientAddr.sin_port)
            {
                isNewClient = false;
                break;
            }
        }
        // Nếu là client mới, thêm vào danh sách và thông báo
        if (isNewClient)
        {
            clientList.push_back(clientAddr);
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
            cout << "New client joined: " << ipStr << ":" << ntohs(clientAddr.sin_port) << '\n';
        }

        for (auto& client : clientList)
        {
            if (client.sin_addr.s_addr != clientAddr.sin_addr.s_addr ||
                client.sin_port != clientAddr.sin_port)
            {
                sendto(serverSocket, buffer, ret, 0, (sockaddr*)&client, clientAddrLen);
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
