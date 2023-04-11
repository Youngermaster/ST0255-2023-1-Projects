#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "json.hpp"
using json = nlohmann::json;

#define REQUEST_TIMEOUT 5  // 5 seconds timeout
#define LOG(x) std::cout << x << std::endl
#define BUFFER_SIZE 4096
#define PORT 8080

std::string get_content_type(const std::string &filename) {
    std::string ext = filename.substr(filename.find_last_of(".") + 1);

    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "ico") return "image/x-icon";
    if (ext == "json") return "application/json";
    if (ext == "txt") return "text/plain";
    if (ext == "xml") return "text/xml";

    return "application/octet-stream";
}

class HttpServer {
   public:
    HttpServer(const std::string &address, int port);
    ~HttpServer();
    void start();
    void on(const std::string &method, const std::string &path, const std::function<void(int, const std::map<std::string, std::string> &, const std::string &)> &handler);

   private:
    std::string address;
    int port;
    int server_fd;
    std::map<std::pair<std::string, std::string>, std::function<void(int, const std::map<std::string, std::string> &, const std::string &)>> handlers;
    void accept_connections();
    void handle_client(int client_fd);
};

HttpServer::HttpServer(const std::string &address, int port) : address(address), port(port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

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

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

HttpServer::~HttpServer() {
    close(server_fd);
}

void HttpServer::start() {
    std::thread t(&HttpServer::accept_connections, this);
    t.detach();
}

void HttpServer::on(const std::string &method, const std::string &path, const std::function<void(int, const std::map<std::string, std::string> &, const std::string &)> &handler) {
    handlers[{method, path}] = handler;
}

void HttpServer::accept_connections() {
    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = REQUEST_TIMEOUT;
        timeout.tv_usec = 0;

        if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
            perror("setsockopt");
            close(client_fd);
            continue;
        }

        std::thread t([this, client_fd]() {
            handle_client(client_fd);
        });
        t.detach();
    }
}

void HttpServer::handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    std::string request_headers;

    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) {
        perror("recv");
        close(client_fd);
        return;
    }
    request_headers.append(buffer, bytes_read);

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

    auto handler_it = handlers.find({method, path});
    if (handler_it == handlers.end()) {
        handler_it = handlers.find({method, "*"});
        if (handler_it == handlers.end()) {
            handler_it = handlers.find({"*", path});
        }
    }
    if (handler_it != handlers.end()) {
        handler_it->second(client_fd, headers, path);
    } else {
        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, response.data(), response.size(), 0);
    }

    close(client_fd);
}

void post_handler(int client_fd, const std::map<std::string, std::string> &headers, const std::string &path) {
    auto content_length_it = headers.find("Content-Length");
    ssize_t content_length = 0;
    if (content_length_it != headers.end()) {
        content_length = std::stoi(content_length_it->second);
    } else {
        std::string response = "HTTP/1.1 411 Length Required\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, response.data(), response.size(), 0);
        LOG(response.data());
        return;
    }

    std::vector<char> buffer(content_length);
    ssize_t bytes_received = 0;
    while (bytes_received < content_length) {
        ssize_t result = read(client_fd, buffer.data() + bytes_received, content_length - bytes_received);
        if (result < 0) {
            std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            send(client_fd, response.data(), response.size(), 0);
            LOG(response.data());
            return;
        } else if (result == 0) {
            // The connection has been closed by the client.
            break;
        }
        bytes_received += result;
    }

    std::string body(buffer.begin(), buffer.begin() + bytes_received);

    // Log the received body
    LOG("Received body: ");
    LOG(body);
    // Process the POST request body
    try {
        json request_json = json::parse(body);
        std::string name = request_json["name"];

        json response_json;
        response_json["message"] = "Hello, " + name;

        std::string response_body = response_json.dump();
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n" + response_body;
        send(client_fd, response.data(), response.size(), 0);
        LOG(response.data());
    } catch (const std::exception &e) {
        std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, response.data(), response.size(), 0);
        LOG(response.data());
    }
}

int main(int argc, char const *argv[]) {
    HttpServer server("127.0.0.1", PORT);

    server.on("POST", "/api/name", post_handler);

    server.on("GET", "*", [](int client_fd, const std::map<std::string, std::string> &headers, const std::string &path) {
        std::string filename = path == "/" ? "index.html" : path.substr(1);
        std::ifstream file(filename, std::ios::binary);

        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string body = buffer.str();
            std::string content_type = get_content_type(filename);

            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
            send(client_fd, response.data(), response.size(), 0);
            LOG(response.data());
        } else {
            std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            LOG(response.data());
            send(client_fd, response.data(), response.size(), 0);
        }
    });

    server.on("HEAD", "*", [](int client_fd, const std::map<std::string, std::string> &headers, const std::string &path) {
        std::string filename = path == "/" ? "index.html" : path.substr(1);
        std::ifstream file(filename, std::ios::binary);

        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string body = buffer.str();
            std::string content_type = get_content_type(filename);

            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n";
            send(client_fd, response.data(), response.size() - body.size(), 0);
            LOG(response.data());
        } else {
            std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            LOG(response.data());
            send(client_fd, response.data(), response.size(), 0);
        }
    });

    server.start();

    LOG("Server started at http://127.0.0.1:8080");

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}