#include<iostream>
#include<WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int main() {
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);

	if (WSAStartup(wVersion, &wsaData)) {
		cout << "Version is not supported" << '\n';
	}

	cout << "Client started" << '\n';

	SOCKET client;
	client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	int tv = 10000;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(5500);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

	char buff[1024];
	int ret, serverAddrLen = sizeof(serverAddr);

	do {
		cout << "Send to server: ";
		gets_s(buff, 1024);

		ret = sendto(client, buff, size_t(strlen(buff)), 0, (sockaddr*)&serverAddr, serverAddrLen);

		if (ret == SOCKET_ERROR) {
			cout << "Error! Cannot send message" << '\n';
		}

		ret = recvfrom(client, buff, 1024, 0, (sockaddr*)&serverAddr, &serverAddrLen);

		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				cout << "Time-out!" << '\n';
			}
			else {
				cout << "Error! Cannot receive message";
			}
		}
		else {
			buff[ret] = '\0';
			cout << "Receive from server: " << buff << '\n';
		}
		_strupr_s(buff, 1024);
	} while (strcmp(buff, "BYE") != 0);

	closesocket(client);
	WSACleanup();
}