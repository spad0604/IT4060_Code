#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<iostream>
#include<fstream>

const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 5000;
const int BUFFER_SIZE = 1024;

int udpSocket;

void sigHandler(int sig) {
    printf("Client is shutting down...\n");
    close(udpSocket);
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, sigHandler);
    
    struct sockaddr_in serverAddr;
    socklen_t serverLen = sizeof(serverAddr);
    char buffer[BUFFER_SIZE];
    
    // Check command line arguments
    if (argc != 3) {
        printf("Usage: %s <command> <filename>\n", argv[0]);
        printf("Commands:\n");
        printf("  get - Get a file from server\n");
        return 1;
    }
    
    // Create socket
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure timeout for recvfrom
    struct timeval tv;
    tv.tv_sec = 5;  // 5 seconds timeout
    tv.tv_usec = 0;
    setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Configure server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Process command
    if (strcmp(argv[1], "get") == 0) {
        char* filename = argv[2];
        
        // Prepare GET request
        char request[BUFFER_SIZE];
        sprintf(request, "GET:%s", filename);
        
        // Send request to server
        if (sendto(udpSocket, request, strlen(request), 0,
                 (struct sockaddr*)&serverAddr, serverLen) < 0) {
            perror("Failed to send request");
            close(udpSocket);
            return 1;
        }
        
        printf("Sent file request for '%s'\n", filename);
        
        // Receive server response
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&serverAddr, &serverLen);
        
        if (bytesReceived < 0) {
            perror("No response from server (timeout)");
            close(udpSocket);
            return 1;
        }
        
        // Check response
        if (strncmp(buffer, "ERROR:", 6) == 0) {
            printf("Server error: %s\n", buffer + 6);
            close(udpSocket);
            return 1;
        }
        
        // Parse file size
        long filesize = 0;
        if (strncmp(buffer, "SIZE:", 5) == 0) {
            filesize = atol(buffer + 5);
            printf("File size: %ld bytes\n", filesize);
            
            // Send READY acknowledgment
            strcpy(buffer, "READY");
            sendto(udpSocket, buffer, strlen(buffer), 0,
                  (struct sockaddr*)&serverAddr, serverLen);
            
            // Create output file
            FILE* outfile = fopen(filename, "wb");
            if (outfile == NULL) {
                perror("Failed to create output file");
                close(udpSocket);
                return 1;
            }
            
            // Receive file data
            int expected_packet = 0;
            long total_bytes = 0;
            bool transfer_complete = false;
            
            printf("Receiving file...\n");
            
            while (!transfer_complete) {
                memset(buffer, 0, BUFFER_SIZE);
                bytesReceived = recvfrom(udpSocket, buffer, BUFFER_SIZE, 0,
                                        (struct sockaddr*)&serverAddr, &serverLen);
                
                if (bytesReceived < 0) {
                    perror("Receive timeout");
                    fclose(outfile);
                    close(udpSocket);
                    return 1;
                }
                
                // Check for END message
                if (strcmp(buffer, "END") == 0) {
                    transfer_complete = true;
                    printf("Transfer complete!\n");
                    sendto(udpSocket, "ACK", 3, 0,
                          (struct sockaddr*)&serverAddr, serverLen);
                    continue;
                }
                
                // Parse packet header
                char* data_start = strchr(buffer, ':');
                if (data_start == NULL) {
                    printf("Invalid packet format\n");
                    sendto(udpSocket, "NACK", 4, 0,
                          (struct sockaddr*)&serverAddr, serverLen);
                    continue;
                }
                
                // Extract packet ID and data
                int packet_id = atoi(buffer);
                data_start++; // Skip the ':'
                
                int data_len = bytesReceived - (data_start - buffer);
                
                // Check if packet is in order
                if (packet_id == expected_packet) {
                    // Write data to file
                    fwrite(data_start, 1, data_len, outfile);
                    total_bytes += data_len;
                    
                    // Display progress
                    float progress = (float)total_bytes / filesize * 100.0;
                    printf("\rProgress: %.1f%% (%ld/%ld bytes)", 
                           progress, total_bytes, filesize);
                    fflush(stdout);
                    
                    // Send ACK
                    sprintf(buffer, "ACK:%d", packet_id);
                    sendto(udpSocket, buffer, strlen(buffer), 0,
                          (struct sockaddr*)&serverAddr, serverLen);
                    
                    expected_packet++;
                } else {
                    // Out of order packet - request retransmission
                    printf("\nOut of order packet (got %d, expected %d)\n", 
                           packet_id, expected_packet);
                    sprintf(buffer, "NACK:%d", expected_packet);
                    sendto(udpSocket, buffer, strlen(buffer), 0,
                          (struct sockaddr*)&serverAddr, serverLen);
                }
            }
            
            printf("\nFile received successfully: %ld bytes\n", total_bytes);
            fclose(outfile);
        }
    } else {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }
    
    close(udpSocket);
    return 0;
} 