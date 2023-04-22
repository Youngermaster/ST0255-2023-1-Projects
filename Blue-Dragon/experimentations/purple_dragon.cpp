#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

const int PORT = 6969;
const bool DEBUG = true;  // Set to true to enable logging, false to disable

void log(const std::string& message) {
    if (DEBUG) {
        std::cout << message << std::endl;
    }
}

std::string get_content_type(const std::string& ext) {
    if (ext == ".html") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    return "application/octet-stream";
}

std::string get_file_extension(const std::string& path) {
    std::size_t pos = path.find_last_of('.');
    return (pos == std::string::npos) ? "" : path.substr(pos);
}

std::map<std::string, std::string> parse_headers(const std::string& raw_headers) {
    std::map<std::string, std::string> headers;
    std::istringstream header_stream(raw_headers);
    std::string line;

    while (std::getline(header_stream, line)) {
        std::size_t colon_pos = line.find(':');

        if (colon_pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 2);

        headers[key] = value;
    }

    return headers;
}

void send_response(int client_socket, const std::string& status, const std::string& content_type, const std::string& content, bool include_body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "\r\n";

    if (include_body) {
        response << content;
    }

    send(client_socket, response.str().c_str(), response.str().size(), 0);
    // Log the response
    log("Sent response:\n" + response.str());
}

void handle_client(int client_socket) {
    char buffer[4096];
    memset(buffer, 0, 4096);

    int bytes_received = recv(client_socket, buffer, 4096, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }

    std::istringstream request(buffer);
    std::string request_type, path, http_version, raw_headers, line;
    request >> request_type >> path >> http_version;

    std::getline(request, line);  // Consume the remaining part of the first line
    while (std::getline(request, line) && line != "\r") {
        raw_headers += line + "\n";
    }

    if (request_type == "POST") {
        auto headers = parse_headers(raw_headers);
        int content_length = std::stoi(headers["Content-Length"]);

        std::vector<char> post_data(content_length);
        request.read(post_data.data(), content_length);
        std::string post_data_str(post_data.begin(), post_data.end());

        // Process the submitted form data as needed (e.g., store in a database)

        // In this example, the submitted form data is simply echoed back to the client
        send_response(client_socket, "200 OK", "text/plain", post_data_str, true);
    } else if (request_type == "GET" || request_type == "HEAD") {
        if (path == "/") {
            path = "/index.html";
        }

        std::ifstream file(path.substr(1), std::ios::binary);
        std::string content;

        if (file.is_open()) {
            content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            send_response(client_socket, "200 OK", get_content_type(get_file_extension(path)), content, request_type != "HEAD");
        } else {
            content = "File not found.";
            send_response(client_socket, "404 Not Found", "text/plain", content, request_type != "HEAD");
        }
    } else {
        std::string content = "Method not allowed.";
        send_response(client_socket, "405 Method Not Allowed", "text/plain", content, request_type != "HEAD");
    }

    close(client_socket);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Can't create a socket" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Can't bind to IP/port" << std::endl;
        return 2;
    }

    if (listen(server_socket, SOMAXCONN) == -1) {
        std::cerr << "Can't listen for connections" << std::endl;
        return 3;
    }

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);

        if (client_socket == -1) {
            std::cerr << "Can't accept client connection" << std::endl;
            continue;
        }

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    return 0;
}
