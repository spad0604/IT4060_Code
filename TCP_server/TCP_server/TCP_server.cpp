#include<stdio.h>
#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>

#define SERVER_PORT 5500
#define SERVER_ADDR "127.0.0.1"

#define BUFF_SIZE 2048
#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char* argv[]) {

	//Step 1: Initiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		std::cout << "Winsock 2.2 is not supported" << '\n';
		return 0;
	}

	//Step 2: Construct socket
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		std::cout << "Error: " << WSAGetLastError() << ": Cannot create server socket" << '\n';
		return 0;
	}

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		std::cout << "Error: " << WSAGetLastError() << " Cannot associate a local address with server socket.";
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		std::cout << "Error: " << WSAGetLastError() << " Cannot server socket in state Listen.";
		return 0;
	}

	std::cout << "Server started" << '\n';

	//Step 5: Communicate with client
	sockaddr_in clientAddr;
	char buff[BUFF_SIZE], clientIP[INET_ADDRSTRLEN];
	int ret, clientAddrLen = sizeof(clientAddr), clientPort;
	SOCKET connSock;
	
	//Accept request
	connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
	if (connSock == SOCKET_ERROR) {
		std::cout << "Error: " << WSAGetLastError() << " Cannot permit incoming connection" << '\n';
		return 0;
	}
	else {
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
		clientPort = ntohs(clientAddr.sin_port);
		std::cout << "Accept incoming connection from " << clientIP << ":" << clientPort;
	}

	while (1) {
		ret = recv(connSock, buff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			std::cout << "Error " << WSAGetLastError() << " Cannot receive data.";
			break;
		}
		else {
			if (ret == 0) {
				std::cout << "Client disconnect." << '\n';
				break;
			}
			else {
				buff[ret] = 0;
				std::cout << "Receive from client[" << clientIP << ":" << clientPort << "]" << " " << buff << '\n';

				//Echo to client
				ret = send(connSock, buff, strlen(buff), 0);
				if (ret == SOCKET_ERROR) {
					std::cout << "Error " << WSAGetLastError() << ":" << " Cannot send data" << '\n';
					break;
				}
			}
		}
	} //end communicating

	closesocket(connSock);
	closesocket(listenSock);

	WSACleanup();
	return 0;
}