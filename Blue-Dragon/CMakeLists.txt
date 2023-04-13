# Specifies the minimum required version of CMake for this project
cmake_minimum_required(VERSION 3.10)

# Defines the project name
project(blue_dragon)

# Sets the C++ standard to C++17 and makes it required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Creates an executable target 'blue_dragon_webserver' from the source file 'blue_dragon.cpp' and header file 'http_server.h'
add_executable(blue_dragon_webserver blue_dragon.cpp http_server.h)

# Links the 'pthread' library to the 'blue_dragon_webserver' target
target_link_libraries(blue_dragon_webserver pthread)

# Copies the 'public' folder to the binary directory
file(COPY public DESTINATION ${CMAKE_BINARY_DIR})

# Installs the 'blue_dragon_webserver' binary to the 'bin' folder
install(TARGETS blue_dragon_webserver DESTINATION bin)

# Installs the 'public' folder to the 'bin' folder
install(DIRECTORY public DESTINATION bin)

# Sets the build type to 'Release' if not specified by the user
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Sets the compiler flags based on the build type
if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Adds warning flags and debugging symbols for Debug build
    add_compile_options(-Wall -Wextra -pedantic -g)
else()
    # Adds optimization level 3 for Release build
    add_compile_options(-O3)
endif()
