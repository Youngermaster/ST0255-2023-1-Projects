#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

using namespace std;
const int BUFFER_SIZE = 4096;
const int PORT = 6969;

void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, BUFFER_SIZE);

    // Parse HTTP request
    stringstream ss(buffer);                // Como parsea este metodo?
    string method, url, http_version;
    ss >> method >> url >> http_version;

    // Handle GET requests
    if (method == "GET") {
        // Open requested file
        string filename = url.substr(1);
        ifstream file(filename);

        if (file.good()) {
            // File exists, send 200 response with file contents
            stringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: text/html\r\n";
            response << "Content-Length: " << file.tellg() << "\r\n\r\n";
            file.seekg(0, ios::beg);
            response << file.rdbuf();
            send(client_socket, response.str().c_str(), response.str().length(), 0);
            write(client_socket, response.str().c_str(), response.str().length());

            // Print response code
            cout << "Response code: 200 OK" << endl;
            cout << response.str() << endl;
        } else {
            // File not found, send 404 response
            stringstream response;
            response << "HTTP/1.1 404 Not Found\r\n";
            response << "Content-Type: text/html\r\n";
            response << "Content-Length: 0\r\n\r\n";
            write(client_socket, response.str().c_str(), response.str().length());

            // Print response code
            cout << "Response code: 404 Not Found" << endl;
        }
    }

    // Handle POST requests
    else if (method == "POST") {
        // Read request body
        string body;
        while (ss.peek() != EOF) {
            char c = ss.get();
            body += c;
        }

        // Print request body
        cout << "Received POST request with body: " << body << endl;

        // Send 200 response
        stringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: 0\r\n\r\n";
        write(client_socket, response.str().c_str(), response.str().length());
        cout << response.str() << endl;
    }

    // Handle HEAD requests
    else if (method == "HEAD") {
        // Open requested file
        string filename = url.substr(1);
        ifstream file(filename);

        if (file.good()) {
            // File exists, send 200 response
            stringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: text/html\r\n";
            response << "Content-Length: " << file.tellg() << "\r\n\r\n";
            write(client_socket, response.str().c_str(), response.str().length());
        } else {
            // File not found, send 404 response
            stringstream response;
            response << "HTTP/1.1 404 Not Found\r\n";
            response << "Content-Type: text/html\r\n";
            response << "Content-Length: 0\r\n\r\n";
            write(client_socket, response.str().c_str(), response.str().length());
        }
    }
    // Handle invalid requests
    else {
        // Send 400 response
        stringstream response;
        response << "HTTP/1.1 400 Bad Request\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: 0\r\n\r\n";
        write(client_socket, response.str().c_str(), response.str().length());
    }

    // Close client socket
    close(client_socket);
}

int main() {
    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        cerr << "Failed to create server socket" << endl;
        return 1;
    }

    // Bind server socket to port
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    if (bind(server_socket, (sockaddr*)&server_address, sizeof(server_address)) == -1) {
        cerr << "Failed to bind server socket to port " << PORT << endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, SOMAXCONN) == -1) {
        cerr << "Failed to listen for incoming connections" << endl;
        return 1;
    }

    // Accept incoming connections and spawn new threads to handle them
    while (true) {
        sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_size);

        if (client_socket == -1) {
            cerr << "Failed to accept incoming connection" << endl;
            continue;
        }

        // Spawn new thread to handle connection
        thread(handle_request, client_socket).detach();
    }

    // Close server socket
    close(server_socket);

    return 0;
}