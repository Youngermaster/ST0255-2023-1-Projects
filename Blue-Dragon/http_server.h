// Include the necessary headers for creating a socket, setting up network connections, and performing I/O operations
#include <arpa/inet.h> // Provides functions for working with IP addresses, such as converting them between host and network byte orders.
#include <netinet/in.h> // Defines data structures and constants for working with internet addresses and ports, e.g., sockaddr_in structure.
#include <sys/socket.h> // Provides functions and data structures for creating and working with sockets.
#include <unistd.h> // Provides various POSIX operating system functions, such as close(), which is used for closing file descriptors, including sockets.

// Include headers for C++ standard libraries used in the program
#include <functional> // Provides utilities for working with C++ functions and function objects, such as std::function and std::bind.
#include <map> // Provides the std::map container class, which is a sorted associative container that can store key-value pairs.
#include <string> // Provides the std::string class, which is a versatile and efficient container for manipulating and storing strings.
#include <iostream> // Provides classes for working with input and output streams, such as std::cin, std::cout, and std::cerr.
#include <thread> // Provides the std::thread class and related functions for working with threads in C++.

// Include the header for the JSON library and define a shorter alias for the nlohmann::json namespace
#include "json.hpp"
using json = nlohmann::json;

// Define constants for the server configuration
#define REQUEST_TIMEOUT 5                   // 5 seconds timeout for client requests
#define LOG(x) std::cout << x << std::endl  // Macro for logging messages to the console
#define BUFFER_SIZE 4096                    // Size of the buffer used for reading client requests and sending responses
#define PORT 8080                           // Default port number for the server


// Declare a helper function for getting the content type of a file based on its extension
std::string get_content_type(const std::string &filename);

// Define the HttpServer class
class HttpServer {
   public:
    // Constructor: initializes an HttpServer instance with the given address and port
    HttpServer(const std::string &address, int port);
    // Destructor: cleans up resources used by the HttpServer instance
    ~HttpServer();
    // Method to start the server and accept incoming connections
    void start();
    // Method to register a request handler for a specific HTTP method and path
    void on(const std::string &method, const std::string &path, const std::function<void(int, const std::map<std::string, std::string> &, const std::string &)> &handler);

   private:
    // Instance variables for the server's address, port, and file descriptor
    std::string address;
    int port;
    int server_fd;
    // Map for storing registered request handlers, keyed by pairs of HTTP method and path
    std::map<std::pair<std::string, std::string>, std::function<void(int, const std::map<std::string, std::string> &, const std::string &)>> handlers;
    // Method to accept incoming client connections
    void accept_connections();
    // Method to handle client requests, taking the client's file descriptor as an argument
    void handle_client(int client_fd);
};
