// Include necessary header files for networking and socket programming
#include <netinet/in.h>  // Provides declarations for IP address and port number structures
#include <sys/socket.h>  // Provides socket-related functions and data structures
#include <unistd.h>      // Provides close() function to close sockets

// Include necessary header files for file I/O, strings, containers, and threads
#include <cstring>   // Provides functions for manipulating C-style strings and memory blocks
#include <fstream>   // Provides file stream classes for file I/O
#include <iostream>  // Provides I/O stream objects (cin, cout, cerr)
#include <map>       // Provides std::map container for associative arrays
#include <sstream>   // Provides string stream classes for string manipulation
#include <string>    // Provides std::string class for handling strings
#include <thread>    // Provides std::thread class for creating and managing threads
#include <vector>    // Provides std::vector container for dynamic arrays

// Declare and initialize global variables for debug flag, port number, log file, and static files directory
bool debug_flag = false;         // Default value for DEBUG flag
int port = 8080;                 // Default value for PORT
std::string log_file;            // Default value is an empty string, which means no log file
std::string static_files = ".";  // Default value for static files directory

// Function to log a message to a log file or stdout if the debug flag is set
void log(const std::string& message, const std::string& log_file) {
    // Check if the debug flag is set
    if (debug_flag) {
        // If a log file is specified, write the message to the log file
        if (!log_file.empty()) {
            // Open the log file in append mode
            std::ofstream log_stream(log_file, std::ios_base::app);
            // Write the message to the log file and add a newline
            log_stream << message << std::endl;
        } else {
            // If no log file is specified, write the message to stdout
            std::cout << message << std::endl;
        }
    }
}

// Function to return the content type based on the file extension
std::string get_content_type(const std::string& ext) {
    if (ext == ".html") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    // The application/octet-stream MIME type is used for unknown binary files
    return "application/octet-stream";
}

// Function to return the file extension of a given path
std::string get_file_extension(const std::string& path) {
    // Find the position of the last '.' character in the path
    std::size_t pos = path.find_last_of('.');
    // If '.' is found, return the substring from the position of the '.' to the end of the string
    // If '.' is not found, return an empty string
    return (pos == std::string::npos) ? "" : path.substr(pos);
}

// Function to parse raw HTTP headers into a map
std::map<std::string, std::string> parse_headers(const std::string& raw_headers) {
    // Create a map to store the parsed headers
    std::map<std::string, std::string> headers;
    // Create a string stream from the raw headers string
    std::istringstream header_stream(raw_headers);
    std::string line;

    // Read header lines and split them into key-value pairs
    while (std::getline(header_stream, line)) {
        // Find the position of the ':' character in the line
        std::size_t colon_pos = line.find(':');

        // If ':' is not found, skip the line and continue with the next one
        if (colon_pos == std::string::npos) {
            continue;
        }

        // Extract the header key and value from the line
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 2);

        // Add the key-value pair to the headers map
        headers[key] = value;
    }

    // Return the parsed headers map
    return headers;
}

// Function to send an HTTP response to the client
void send_response(int client_socket, const std::string& status, const std::string& content_type, const std::string& content, bool include_body) {
    // Create an output string stream to build the response
    std::ostringstream response;
    // Add the response status line, content type, and content length headers
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "X Y: " << "ABCD" << "\r\n";
    // Add an empty line to separate headers from the response body
    response << "\r\n";

    // If the 'include_body' flag is true, add the response body to the output stream
    if (include_body) {
        response << content;
    }

    // Send the response to the client using the 'send()' function
    send(client_socket, response.str().c_str(), response.str().size(), 0);
    // Log the sent response using the 'log()' function
    log("Sent response:\n" + response.str(), log_file);
}

// Function to handle an individual client connection
void handle_client(int client_socket, const std::string& log_file, const std::string& static_files) {
    char buffer[4096];
    // Initialize the buffer with zeros to avoid garbage values
    memset(buffer, 0, 4096);

    // Receive data from the client and store it in the buffer
    int bytes_received = recv(client_socket, buffer, 4096, 0);
    // If no bytes are received or an error occurs, close the client socket and return
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }

    // Parse the received data into request type, path, HTTP version, and headers
    std::istringstream request(buffer);
    std::string request_type, path, http_version, raw_headers, line;
    request >> request_type >> path >> http_version;

    // Read header lines from the request stream and concatenate them into the raw_headers string
    std::getline(request, line);  // Consume the remaining part of the first line
    while (std::getline(request, line) && line != "\r") {
        raw_headers += line + "\n";
    }

    // Process POST requests and send a response based on the submitted form data
    if (request_type == "POST") {
        auto headers = parse_headers(raw_headers);
        int content_length = std::stoi(headers["Content-Length"]);

        // Read POST data
        std::vector<char> post_data(content_length);
        request.read(post_data.data(), content_length);
        std::string post_data_str(post_data.begin(), post_data.end());

        // Process the submitted form data as needed (e.g., store in a database)

        // In this example, the submitted form data is simply echoed back to the client
        send_response(client_socket, "200 OK", "text/plain", post_data_str, true);
    }
    // Process GET and HEAD requests, read the requested file, and send the response
    else if (request_type == "GET" || request_type == "HEAD") {
        if (path == "/") {
            path = "/index.html";
        }

        // Read the requested file from the static files directory
        std::string file_path = static_files + path;
        std::ifstream file(file_path, std::ios::binary);
        std::string content;

        if (file.is_open()) {
            content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            send_response(client_socket, "200 OK", get_content_type(get_file_extension(path)), content, request_type != "HEAD");
        } else {
            // Send a 404 response if the file is not found
            content = "File not found.";
            send_response(client_socket, "404 Not Found", "text/plain", content, request_type != "HEAD");
        }
    }
    // For unsupported request types, send a "405 Method Not Allowed" response
    else {
        // Send a 405 response if the method is not allowed
        std::string content = "Method not allowed.";
        send_response(client_socket, "405 Method Not Allowed", "text/plain", content, request_type != "HEAD");
    }

    // Close the client connection after processing the request and sending a response
    close(client_socket);
}

// Main function to set up the server and handle client connections
int main(int argc, char* argv[]) {
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--debug") {
            debug_flag = true;
        } else if (arg == "--logfile" && i + 1 < argc) {
            log_file = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--static-files" && i + 1 < argc) {
            static_files = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return 1;
        }
    }

    // Set the DEBUG flag and update the port
    const bool DEBUG = debug_flag;
    const int PORT = port;

    // Create a server socket
    // Create a new socket using the IPv4 address family (AF_INET) and TCP protocol (SOCK_STREAM)
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Check if the socket was created successfully
    if (server_socket == -1) {
        // Print an error message if the socket creation failed
        std::cerr << "Can't create a socket" << std::endl;

        // Terminate the program with a non-zero exit code to indicate an error
        return 1;
    }

    // Bind the server socket to an IP address and port
    // Create a sockaddr_in structure to store the server address
    sockaddr_in server_addr;
    // Set the address family to IPv4 (AF_INET)
    server_addr.sin_family = AF_INET;
    // Convert the port number from host byte order to network byte order and assign it to the server address
    server_addr.sin_port = htons(PORT);
    // Bind the server socket to any available IP address on the machine (INADDR_ANY)
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Attempt to bind the server socket to the specified IP address and port
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        // Print an error message if the binding process failed
        std::cerr << "Can't bind to IP/port" << std::endl;
        // Terminate the program with a non-zero exit code (2 in this case) to indicate an error
        return 2;
    }

    // Start listening for connections on the server socket, allowing a maximum number of connections specified by SOMAXCONN
    if (listen(server_socket, SOMAXCONN) == -1) {
        // Print an error message if the listening process failed
        std::cerr << "Can't listen for connections" << std::endl;

        // Terminate the program with a non-zero exit code (3 in this case) to indicate an error
        return 3;
    }

    // Main server loop to accept and handle client connections
    while (true) {
        // Create a sockaddr_in structure to store the client's address information
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        // Accept a client connection and retrieve the client's socket and address information
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);

        // Check if the client connection was accepted successfully
        if (client_socket == -1) {
            // Print an error message if the connection could not be accepted
            std::cerr << "Can't accept client connection" << std::endl;
            // Continue to the next iteration of the loop to try accepting another connection
            continue;
        }

        // Create a new thread to handle the client connection using the 'handle_client' function
        std::thread client_thread(handle_client, client_socket, log_file, static_files);
        // Detach the newly created thread, allowing it to run independently of the main thread
        client_thread.detach();
    }

    return 0;
}
