#include "http_server.h"

// HttpServer constructor
HttpServer::HttpServer(const std::string &address, int port) : address(address), port(port) {
    // Create a socket for the server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow reuse of the address and port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections on the socket
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

// HttpServer destructor
HttpServer::~HttpServer() {
    // Close the server socket
    close(server_fd);
}

// Start the server by spawning a thread to accept connections
void HttpServer::start() {
    std::thread t(&HttpServer::accept_connections, this);
    t.detach();
}

// Register a handler for a specific method and path
void HttpServer::on(const std::string &method, const std::string &path, const std::function<void(int, const std::map<std::string, std::string> &, const std::string &)> &handler) {
    handlers[{method, path}] = handler;
}

// Continuously accept incoming connections
void HttpServer::accept_connections() {
    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        // Accept an incoming connection and obtain its file descriptor
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // Set a timeout for receiving data from the client
        struct timeval timeout;
        timeout.tv_sec = REQUEST_TIMEOUT;
        timeout.tv_usec = 0;
        if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
            perror("setsockopt");
            close(client_fd);
            continue;
        }

        // Spawn a thread to handle the connected client
        std::thread t([this, client_fd]() {
            handle_client(client_fd);
        });
        t.detach();
    }
}

// Handle a connected client
void HttpServer::handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    std::string request_headers;

    // Receive the client's request headers
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) {
        perror("recv");
        close(client_fd);
        return;
    }
    request_headers.append(buffer, bytes_read);

    // Parse the method, path, and version from the request
    std::string method, path, version;
    std::map<std::string, std::string> headers;
    std::string::size_type pos = request_headers.find(" ");
    if (pos != std::string::npos) {
        method = request_headers.substr(0, pos);
        request_headers.erase(0, pos + 1);
    }

    pos = request_headers.find(" ");
    if (pos != std::string::npos) {
        path = request_headers.substr(0, pos);
        request_headers.erase(0, pos + 1);
    }

    pos = request_headers.find("\r\n");
    if (pos != std::string::npos) {
        version = request_headers.substr(0, pos);
        request_headers.erase(0, pos + 2);
    }

    // Log the method and path
    LOG("Method: " + method + ", Path: " + path);

    // Parse the remaining request headers
    while ((pos = request_headers.find("\r\n")) != std::string::npos && pos > 0) {
        std::string header_line = request_headers.substr(0, pos);
        request_headers.erase(0, pos + 2);

        pos = header_line.find(":");
        if (pos != std::string::npos) {
            std::string key = header_line.substr(0, pos);
            std::string value = header_line.substr(pos + 1);
            // Trim leading and trailing spaces
            value.erase(0, value.find_first_not_of(" "));
            value.erase(value.find_last_not_of(" ") + 1);
            headers[key] = value;
        }
    }

    // Check if there's a registered handler for the requested method and path
    auto handler_it = handlers.find({method, path});
    if (handler_it == handlers.end()) {
        handler_it = handlers.find({method, "*"});
        if (handler_it == handlers.end()) {
            handler_it = handlers.find({"*", path});
        }
    }
    // If a handler is found, call it with the client file descriptor, headers, and path
    if (handler_it != handlers.end()) {
        handler_it->second(client_fd, headers, path);
    } else {
        // If no handler is found, return a 405 Method Not Allowed response
        // The 405 Method Not Allowed error occurs between a client and a
        // server. This message indicates that the web server has recognized a
        // request from a web browser to access the website but rejects the
        // specific HTTP method.
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, response.data(), response.size(), 0);
    }

    // Close the client connection
    close(client_fd);
}
