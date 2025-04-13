#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <iostream>
#include <fstream>

const char* FILENAME = "log.txt";

// Hàm xử lý UDP - nhận broadcast tên từ client
void UDPServer() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket < 0) {
        std::cout << "UDP socket creation failed" << std::endl;
        exit(1);
    }

    // Cho phép reuse address
    int optval = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    // Thiết lập cho phép broadcast
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));

    struct sockaddr_in udpAddr;
    memset(&udpAddr, 0, sizeof(udpAddr));
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port = htons(6000);
    udpAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) < 0) {
        std::cout << "UDP socket binding failed" << std::endl;
        close(udpSocket);
        exit(1);
    }

    std::cout << "UDP server is running on port 6000" << std::endl;
    
    // Đợi và xử lý các broadcast từ client
    while (true) {
        char buffer[1024] = {0};
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, 
                                  (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

            std::cout << "Nhận được tên từ client: " << buffer << " từ IP: " << clientIP << std::endl;

            // Ghi log vào file với file lock để đảm bảo an toàn
            int fd = open(FILENAME, O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (fd >= 0) {
                // Khóa file để đảm bảo truy cập an toàn
                flock(fd, LOCK_EX);
                
                // Ghi thông tin vào file
                std::string logEntry = "Client " + std::string(buffer) + 
                                      " connected from " + std::string(clientIP) + "\n";
                write(fd, logEntry.c_str(), logEntry.length());
                
                // Mở khóa file
                flock(fd, LOCK_UN);
                close(fd);
            }
        }

        sleep(1); // Đợi 1 giây trước khi tiếp tục nhận
    }

    close(udpSocket);
}

// Hàm xử lý TCP - gửi danh sách client khi được yêu cầu
void TCPServer() {
    int tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpSocket < 0) {
        std::cout << "TCP socket creation failed" << std::endl;
        exit(1);
    }
    
    // Cho phép reuse address
    int optval = 1;
    setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in tcpAddr;
    memset(&tcpAddr, 0, sizeof(tcpAddr));
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_port = htons(5000);
    tcpAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcpSocket, (struct sockaddr*)&tcpAddr, sizeof(tcpAddr)) < 0) {
        std::cout << "TCP socket binding failed" << std::endl;
        close(tcpSocket);
        exit(1);
    }

    if (listen(tcpSocket, SOMAXCONN) < 0) {
        std::cout << "TCP socket listening failed" << std::endl;
        close(tcpSocket);
        exit(1);
    }

    std::cout << "TCP server is running on port 5000" << std::endl;

    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(tcpSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket >= 0) {
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

            std::cout << "New client connected: " << clientIP << std::endl;

            // Đọc file và gửi nội dung cho client
            int fd = open(FILENAME, O_RDONLY);
            if (fd >= 0) {
                // Khóa file để đảm bảo truy cập an toàn
                flock(fd, LOCK_SH);
                
                // Đọc nội dung file
                std::string content;
                char readBuffer[1024];
                ssize_t bytesRead;
                // Đọc từng phần của file
                while ((bytesRead = read(fd, readBuffer, sizeof(readBuffer) - 1)) > 0) {
                    readBuffer[bytesRead] = '\0';
                    content += readBuffer;
                }
                
                // Mở khóa file
                flock(fd, LOCK_UN);
                close(fd);
                
                if (content.empty()) {
                    content = "Không có client nào đang kết nối\n";
                }
                
                // Gửi nội dung cho client
                send(clientSocket, content.c_str(), content.size(), 0);
            } else {
                // Không mở được file
                std::string errorMsg = "Không có client nào đang kết nối\n";
                send(clientSocket, errorMsg.c_str(), errorMsg.size(), 0);
            }
            
            close(clientSocket);
        }
    }

    close(tcpSocket);
}

// Xử lý tín hiệu để đóng sockets một cách an toàn
void sigHandler(int sig) {
    exit(0);
}

int main() {
    // Xử lý tín hiệu
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    
    // Tạo file rỗng nếu chưa tồn tại
    std::ofstream file(FILENAME, std::ios::trunc);
    file.close();

    // Tạo tiến trình UDP
    pid_t udpPid = fork();
    
    if (udpPid < 0) {
        std::cout << "Không thể tạo tiến trình con UDP" << std::endl;
        exit(1);
    } else if (udpPid == 0) {
        // Tiến trình con UDP
        UDPServer();
        exit(0);
    }
    
    // Tạo tiến trình TCP
    pid_t tcpPid = fork();
    
    if (tcpPid < 0) {
        std::cout << "Không thể tạo tiến trình con TCP" << std::endl;
        exit(1);
    } else if (tcpPid == 0) {
        // Tiến trình con TCP
        TCPServer();
        exit(0);
    }
    
    // Tiến trình cha đợi các tiến trình con kết thúc
    int status;
    waitpid(udpPid, &status, 0);
    waitpid(tcpPid, &status, 0);
    
    return 0;
}

