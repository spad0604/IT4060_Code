#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<signal.h>
#include<iostream>
#include<fstream>

const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 5000;
const int BUFFER_SIZE = 1024;

int udpSocket;

void UDPServer() {
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];
    
    // Create socket
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Bind socket to address
    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(udpSocket);
        exit(EXIT_FAILURE);
    }
    
    std::cout << "UDP File Server running at " << SERVER_IP << ":" << SERVER_PORT << std::endl;
    std::cout << "Waiting for file requests..." << std::endl;
    
    while (true) {
        // Clear buffer
        memset(buffer, 0, BUFFER_SIZE);
        
        // Receive request from client
        int bytesReceived = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0, 
                                    (struct sockaddr*)&clientAddr, &clientLen);
        
        if (bytesReceived > 0) {
            std::cout << "Received request from " << inet_ntoa(clientAddr.sin_addr) 
                      << ":" << ntohs(clientAddr.sin_port) << " - " 
                      << buffer << std::endl;
            
            // Check if it's a file request
            if (strncmp(buffer, "GET:", 4) == 0) {
                char* filename = buffer + 4;  // Skip "GET:"
                
                // Try to open the file
                FILE* file = fopen(filename, "rb");
                
                if (file == NULL) {
                    // File not found, send error message
                    char errorMsg[BUFFER_SIZE] = "ERROR:File not found";
                    sendto(udpSocket, errorMsg, strlen(errorMsg), 0,
                          (struct sockaddr*)&clientAddr, clientLen);
                    continue;
                }
                
                // Get file size
                struct stat file_stat;
                stat(filename, &file_stat);
                long filesize = file_stat.st_size;
                
                // Send file size info to client
                char sizeInfo[BUFFER_SIZE];
                sprintf(sizeInfo, "SIZE:%ld", filesize);
                sendto(udpSocket, sizeInfo, strlen(sizeInfo), 0,
                      (struct sockaddr*)&clientAddr, clientLen);
                
                // Wait for acknowledgment
                memset(buffer, 0, BUFFER_SIZE);
                recvfrom(udpSocket, buffer, BUFFER_SIZE, 0, 
                        (struct sockaddr*)&clientAddr, &clientLen);
                
                if (strcmp(buffer, "READY") == 0) {
                    // Start sending file
                    int bytes_read;
                    int packet_id = 0;
                    char packet[BUFFER_SIZE + 8]; // Extra bytes for packet header
                    
                    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE - 8, file)) > 0) {
                        // Prepare packet with header (packet_id:data)
                        sprintf(packet, "%d:", packet_id);
                        memcpy(packet + strlen(packet), buffer, bytes_read);
                        
                        // Send packet
                        sendto(udpSocket, packet, bytes_read + strlen(packet), 0,
                              (struct sockaddr*)&clientAddr, clientLen);
                        
                        // Wait for ACK
                        memset(buffer, 0, BUFFER_SIZE);
                        recvfrom(udpSocket, buffer, BUFFER_SIZE, 0, 
                                (struct sockaddr*)&clientAddr, &clientLen);
                        
                        packet_id++;
                    }
                    
                    // Send END message
                    strcpy(buffer, "END");
                    sendto(udpSocket, buffer, strlen(buffer), 0,
                          (struct sockaddr*)&clientAddr, clientLen);
                    
                    fclose(file);
                    std::cout << "File transfer completed. Sent " << packet_id << " packets." << std::endl;
                }
            } else {
                // Unknown command
                char response[BUFFER_SIZE] = "Unknown command";
                sendto(udpSocket, response, strlen(response), 0,
                      (struct sockaddr*)&clientAddr, clientLen);
            }
        }
    }
}

void sigHandler(int sig) {
    printf("Server is shutting down...\n");
    close(udpSocket);
    exit(0);
}

int main() {
    signal(SIGINT, sigHandler);
    UDPServer();
    return 0;
}
