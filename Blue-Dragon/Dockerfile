# Use the official gcc image as a base
FROM gcc:latest

# Set the working directory
WORKDIR /app

# Copy the CMakeLists.txt and source files into the container
COPY . .

# Install CMake, wget, ca-certificates, and any other necessary dependencies
RUN apt-get update && \
    apt-get install -y cmake wget ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Create a build directory
RUN mkdir build

# Change to the build directory, build the project, and install the binary and the public folder to the 'bin' folder
RUN cd build && \
    cmake .. && \
    cmake --build . --config Release

# Expose the port your server will run on (change to your desired port)
EXPOSE 8080

# Run the server with the working directory set explicitly
CMD ["sh", "-c", "cd /app/build/ && ./blue_dragon_webserver --debug --logfile logs.txt --static-files ./public --port 8080"]
