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

#define REQUEST_TIMEOUT 5 // 5 seconds timeout
#define LOG(x) std::cout << x << std::endl
#define BUFFER_SIZE 4096
#define PORT 8080

std::string get_content_type(const std::string &filename);

class HttpServer
{
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
