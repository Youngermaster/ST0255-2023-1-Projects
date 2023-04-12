"""
-O3 -> Optimize yet more.  -O3 turns on all optimizations specified by -O2 and
    also turns on the -finline-functions, -funswitch-loops,
    -fpredictive-commoning, -fgcse-after-reload, -ftree-vectorize
    and -fipa-cp-clone options.
-std:c++17 -> Enables C++17 standard-specific features and behavior.
    It enables the full set of C++17 features
"""

echo "Compiling Blue Dragon Project..."

g++ -pthread -std=c++17 -O3 -o webserver blue_dragon.cpp http_server.h
