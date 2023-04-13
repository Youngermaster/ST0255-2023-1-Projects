#include "http_server.cpp"

// Function to determine the content type of a file based on its extension
std::string get_content_type(const std::string &filename) {
    std::string ext = filename.substr(filename.find_last_of(".") + 1);

    // Return the appropriate content type based on the file extension
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


// POST handler for processing JSON data in the request body
void post_handler(int client_fd, const std::map<std::string, std::string> &headers, const std::string &path) {
    LOG("Entré en el POST");
    auto content_length_it = headers.find("Content-Length");
    ssize_t content_length = 0;
    if (content_length_it != headers.end()) {
        content_length = std::stoi(content_length_it->second);
    } else {
        // If Content-Length is not provided, return a 411 Length Required response
        std::string response = "HTTP/1.1 411 Length Required\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, response.data(), response.size(), 0);
        LOG(response.data());
        return;
    }

    // Create a buffer to receive the request body
    std::vector<char> buffer(content_length);
    ssize_t bytes_received = 0;
    // Read the request body from the client connection
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

    // Convert the received data into a string
    std::string body(buffer.begin(), buffer.begin() + bytes_received);

    // Log the received body
    LOG("Received body: ");
    LOG(body);

    // Process the POST request body
    try {
        // Parse the JSON from the request body
        json request_json = json::parse(body);
        std::string name = request_json["name"];

        // Create a JSON object for the response
        json response_json;
        response_json["message"] = "Hello, " + name;

        // Convert the JSON object to a string
        std::string response_body = response_json.dump();
        // Send a 200 OK response with the JSON object
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n" + response_body;
        send(client_fd, response.data(), response.size(), 0);
        LOG(response.data());
    } catch (const std::exception &e) {
        // If an error occurs while processing the JSON, return a 400 Bad Request response
        std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, response.data(), response.size(), 0);
        LOG(response.data());
    }
}

int main(int argc, char const *argv[]) {
    // Create an HTTP server instance with the specified address and port
    HttpServer server("127.0.0.1", PORT);

    // Register a POST handler for the "/api/name" endpoint
    server.on("POST", "/api/name", post_handler);

    // Register a catch-all GET handler
    server.on("GET", "*", [](int client_fd, const std::map<std::string, std::string> &headers, const std::string &path) {
        std::string filename = path == "/" ? "index.html" : path.substr(1);
        std::ifstream file(filename, std::ios::binary);

        // If the requested file is found, return its content
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string body = buffer.str();
            std::string content_type = get_content_type(filename);

            // Send a 200 OK response with the file content
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
            send(client_fd, response.data(), response.size(), 0);
            LOG(response.data());
        } else if (!file.is_open()) {
            // If the requested file is not found, return a 404 Not Found response
            std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            LOG(response.data());
            send(client_fd, response.data(), response.size(), 0);
        } else {
            // 400 Bad Request response status code indicates that the server cannot or will not process the request due to something that is perceived to be a client error
            std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            LOG(response.data());
            send(client_fd, response.data(), response.size(), 0);
        }
    });

    // Register a catch-all HEAD handler
    server.on("HEAD", "*", [](int client_fd, const std::map<std::string, std::string> &headers, const std::string &path) {
        std::string filename = path == "/" ? "index.html" : path.substr(1);
        std::ifstream file(filename, std::ios::binary);

        // If the requested file is found, return its metadata
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string body = buffer.str();
            std::string content_type = get_content_type(filename);

            // Send a 200 OK response with the file metadata (without the body)
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n";
            send(client_fd, response.data(), response.size() - body.size(), 0);
            LOG(response.data());
        } else if (!file.is_open()) {
            // If the requested file is not found, return a 404 Not Found response
            std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            LOG(response.data());
            send(client_fd, response.data(), response.size(), 0);
        } else {
            // 400 Bad Request response status code indicates that the server cannot or will not process the request due to something that is perceived to be a client error
            std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            LOG(response.data());
            send(client_fd, response.data(), response.size(), 0);
        }
    });

    // Start the server
    server.start();

    // Log the server's address and port
    LOG("Server started at http://127.0.0.1:8080");

    // Keep the main thread running indefinitely
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Return 0 to indicate successful execution (this line will never be reached)
    return 0;
}
