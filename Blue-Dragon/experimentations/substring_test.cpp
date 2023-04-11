#include <iostream>
#include <string>

#define LOG(x) std::cout << x << std::endl

int main(int argc, char const *argv[]) {
    std::string str = "We think in generalities, but we live in details.";
    // (quoting Alfred N. Whitehead)

    std::string str_2 = str.substr(3, 5);  // "think"
    std::string str_3 = str.substr(1);
    LOG(str);
    LOG(str_2);
    LOG(str_3);

    std::string url_path = "/public/index.html";
    LOG(url_path.substr(1));

    return 0;
}
