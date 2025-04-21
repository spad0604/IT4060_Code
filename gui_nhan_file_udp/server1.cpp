#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <iostream>
#include <fstream>

const char *SERVER_IP = "127.0.0.1";
const char *PORT = "8181";
const int BUFFER_SIZE = 1024;

int udpSocket;

void UDPServer()
{
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0)
    {
        std::cout << "Socket creation failed";
        exit(0);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Bind
    if (bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr) < 0))
    {
        std::cout << "Bind failed";
        close(udpSocket);
        exit(0);
    }

    std::cout << "UDP File Server is running at " << SERVER_IP << ":" << PORT << std::endl;
    std::cout << "Waiting for file requests..." << std::endl;

    while (1)
    {
        // Clear buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Receive request from client
        int bytesReceived = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clientLen);

        if (bytesReceived > 0)
        {
            std::cout << "Received from" << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << "-" << buffer << '\n';
        }

        if (strncmp(buffer, "GET:", 4) == 0)
        {
            char *fileName = buffer + 4;

            FILE *file = fopen(fileName, "rb");

            if (file == NULL)
            {
                char errorMsg[BUFFER_SIZE] = "ERROR:File not found";
                sendto(udpSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAddr, &clientLen);
                continue;
            }

            // GET file size
            struct stat file_stat;
            stat(fileName, &file_stat);
            long fileSize = file_stat.st_size;

            char sizeInfo[BUFFER_SIZE];
            sprintf(sizeInfo, "SIZE:%ld", fileSize);
            sendto(udpSocket, sizeInfo, strlen(sizeInfo), 0, (struct sockaddr *)&clientAddr, clientLen);

            memset(buffer, 0, BUFFER_SIZE);
            recvfrom(udpSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clientLen);

            if (strcmp(buffer, "READY") == 0)
            {
                int bytes_read;
                int packet_id = 0;
                char packet[BUFFER_SIZE + 8]; // Extra bytes for packet header

                while ((bytes_read == fread(buffer, 1, BUFFER_SIZE - 8, file)) > 0)
                {
                    // Prepare packet with header (packet_id: data)
                    sprintf(packet, "%d", packet_id);
                    memccpy(packet + strlen(packet), buffer, bytes_read);

                    // Send packet
                    sendto(udpSocket, packet, bytes_read + strlen(packet), 0, (struct sockaddr *)&clientAddr, clientLen);

                    // Wait for ACK
                    memset(buffer, 0, BUFFER_SIZE);
                    recvfrom(udpSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &clientLen);

                    packet_id++;
                }

                //Send END message
                strcpy(buffer, "END");
                sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr*)&clientAddr, clientLen);

                fclose(file);
                std::cout << "File transfer completed. Sent" << packet_id << " packets." << '\n';
            } else {
                //Unknown command
                char response[BUFFER_SIZE], = "Unknown command";
                sendto(udpSocket, response, strlen(response), 0, (struct sockaddr*)&clientAddr, clientLen);
            }
        }
    }
}

void sigHandler(int sig) {
    std::cout << "Server is shutting down...\n";
    close(udpSocket);
    exit(0);
}

int main() {
    signal(SIGINT, sigHandler);
    UDPServer();
    return 0;
}