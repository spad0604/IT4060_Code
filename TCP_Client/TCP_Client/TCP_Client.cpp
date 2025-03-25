#include<stdio.h>
#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>

#define SERVER_PORT 5500
#define SERVER_ADDR "127.0.0.1"

#define BUFF_SIZE 2048
#pragma comment(lib, "Ws2_32.lib")

int main()
{

	//Step 1: Initiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		std::cout << "Winsock 2.2 is not supported" << '\n';
		return 0;
	}

	//Step 2: Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		std::cout << "Error: " << WSAGetLastError() << ": Cannot create server socket" << '\n';
		return 0;
	}

	//(optional) Set time-out for receiving
	int tv = 10000;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	//Step 3: Specify server Address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	//Step 4: Request to connect server
	if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		std::cout << "Error " << WSAGetLastError() << " Cannot connect server";
		return 0;
	}

	std::cout << "Connected server!" << '\n';

	char buff[BUFF_SIZE];
	int ret, messageLen;

	while (1) {
		std::cout << "Send to server: ";
		gets_s(buff, BUFF_SIZE);
		messageLen = strlen(buff);

		if (messageLen == 0) break;

		ret = send(client, buff, messageLen, 0);

		if (ret == SOCKET_ERROR) {
			std::cout << "Error " << WSAGetLastError() << " Cannot send data" << '\n';
		}

		ret = recv(client, buff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				std::cout << "Time-out!" << '\n';
			}
			else {
				std::cout << "Error " << WSAGetLastError() << " Cannot receive data";
			}
		}
		else {
			if (strlen(buff) > 0) {
				buff[ret] = 0;
				std::cout << "Received from server: " << buff << '\n';
			}
		}
	}

	closesocket(client);
	WSACleanup();

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
