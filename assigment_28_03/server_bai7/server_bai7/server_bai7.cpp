#include<WinSock2.h>
#include<iostream>
#include<string.h>

const std::string getServerIP() {
	return "127.0.0.1";
}

const std::string listFile[] = {
	"test1.txt",
	"test2.txt",
	"test3.txt",
	"test4.txt"
}

int main() {
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
		std::cout << "This version does not support";
		return 0;
	}

	SOCKADDR serverAddr;
	SOCKET_ADDRESS 
}
