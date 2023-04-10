import socket
import threading

# Define the server's host and port
HOST = '127.0.0.1'
PORT = 8080

# Define a dictionary of HTTP status codes and messages
HTTP_STATUS_CODES = {
    200: 'OK',
    400: 'Bad Request',
    404: 'Not Found'
}

# Define a function to handle incoming client requests
def handle_request(client_socket, client_address):
    # Receive the client's request
    request = client_socket.recv(1024).decode()
    
    # Parse the request to get the HTTP method and path
    method, path, _ = request.split(' ', 2)
    
    # Handle the request based on the HTTP method and path
    if method == 'GET':
        # Check if the requested file exists
        try:
            with open(path[1:], 'rb') as file:
                # Read the file contents and send them to the client
                response = b'HTTP/1.1 200 OK\r\n\r\n' + file.read()
                client_socket.sendall(response)
        except FileNotFoundError:
            # If the file doesn't exist, send a 404 error to the client
            response = b'HTTP/1.1 404 Not Found\r\n\r\n'
            client_socket.sendall(response)
    elif method == 'POST':
        # Receive the client's data and print it
        data = client_socket.recv(1024).decode()
        print(f'Received data from {client_address}: {data}')
        
        # Send a 200 OK response to the client
        response = b'HTTP/1.1 200 OK\r\n\r\n'
        client_socket.sendall(response)
    elif method == 'HEAD':
        # Check if the requested file exists
        try:
            with open(path[1:], 'rb') as file:
                # Send a 200 OK response to the client without the file contents
                response = b'HTTP/1.1 200 OK\r\n\r\n'
                client_socket.sendall(response)
        except FileNotFoundError:
            # If the file doesn't exist, send a 404 error to the client
            response = b'HTTP/1.1 404 Not Found\r\n\r\n'
            client_socket.sendall(response)
    else:
        # If the HTTP method is not supported, send a 400 error to the client
        response = b'HTTP/1.1 400 Bad Request\r\n\r\n'
        client_socket.sendall(response)
    
    # Close the connection with the client
    client_socket.close()

# Create a socket object and bind it to the server's host and port
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))

# Start listening for incoming client connections
server_socket.listen()

# Loop indefinitely to handle incoming client connections
while True:
    # Accept a client connection and start a new thread to handle the request
    client_socket, client_address = server_socket.accept()
    threading.Thread(target=handle_request, args=(client_socket, client_address)).start()
