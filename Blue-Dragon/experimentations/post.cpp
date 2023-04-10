#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

int main() {
    // Create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Bind the socket to a port
    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to port" << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Failed to listen for incoming connections" << std::endl;
        return 1;
    }

    while (true) {
        // Accept a new connection
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            std::cerr << "Failed to accept incoming connection" << std::endl;
            continue;
        }

        // Read the request
        char buffer[1024] = {0};
        int bytes_read = read(client_fd, buffer, sizeof(buffer));
        if (bytes_read < 0) {
            std::cerr << "Failed to read request" << std::endl;
            close(client_fd);
            continue;
        }

        // Check if the request is a POST
        if (strncmp(buffer, "POST ", 5) == 0) {
            // Send back the POST data as a response
            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(bytes_read) + "\r\n\r\n" + buffer;
            if (send(client_fd, response.c_str(), response.size(), 0) < 0) {
                std::cerr << "Failed to send response" << std::endl;
                close(client_fd);
                continue;
            }
        } else {
            // Send back a 404 Not Found response for other requests
            std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            if (send(client_fd, response.c_str(), response.size(), 0) < 0) {
                std::cerr << "Failed to send response" << std::endl;
                close(client_fd);
                continue;
            }
        }

        // Close the connection
        close(client_fd);
    }

    return 0;
}
