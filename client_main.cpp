#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

std::atomic<bool> running(true);
std::mutex cout_mutex;
std::mutex connection_mutex;
bool connected = false;

void receiveLoop(int client_fd) {
    char buffer[512];
    while (running) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            char *str = strstr(buffer, "OK connected");
            if (str)
            {
                std::lock_guard<std::mutex> lock(connection_mutex);
                connected = true;
            }
            std::cout << std::string(buffer, bytes);
        }
        else if (bytes == 0) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            running = false;
            break;
        }
        else {
            if (running) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                perror("recv");
            }
            running = false;
            break;
        }
    }
}

int main() {
    const char* server_ip = "127.0.0.1";
    const int server_port = 6667;
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_fd);
        return 1;
    }
    if (connect(client_fd, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cout << "ERR 900 CONNECTION_FAILED\r\n";
        close(client_fd);
        return 1;
    }

    std::cout << "Connected to " << server_ip << ":" << server_port << "\n";

    std::thread receiver(receiveLoop, client_fd);
    while (running) {
        std::string message;
        if (!std::getline(std::cin, message)) {
            running = false;
            break;
        }
        message += "\r\n";
        send(client_fd, message.c_str(), message.size(), 0 );
        if (!running) {
            std::cout << "ERR 901 SEND_FAILED\r\n";
            break;
        }
        if (message == "QUIT\r\n")
        {
            running = false;
            if (connected)
            {
                char buffer[512];
                ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0 );
                std::cout << std::string(buffer, bytes);
            }
            break;
        }
    }
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    if (receiver.joinable()) {
        receiver.join();
    }
    std::cout << "Client closed.\n";
    return 0;
}
