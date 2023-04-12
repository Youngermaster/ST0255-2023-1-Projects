cmake_minimum_required(VERSION 3.10)

project(webserver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(webserver blue_dragon.cpp http_server.h)

target_link_libraries(webserver pthread)

# Copy the 'public' folder to the binary directory
file(COPY public DESTINATION ${CMAKE_BINARY_DIR})

# Install the binary and the public folder
install(TARGETS webserver DESTINATION bin)
install(DIRECTORY public DESTINATION bin)

# Set the build type to Release by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-Wall -Wextra -pedantic -g)
else()
    add_compile_options(-O3)
endif()
