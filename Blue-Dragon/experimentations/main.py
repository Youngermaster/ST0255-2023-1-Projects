import socket
import threading
from urllib.parse import parse_qs


def handle_request(client_socket, client_address):
    # Receive the client's request
    request = client_socket.recv(4096).decode()
    print(f'Received request from {client_address}:\n{request}')

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
        # Receive the client's data and parse it
        content_length = int(request.split(
            'Content-Length: ')[1].split('\r\n')[0])
        data = b''
        while len(data) < content_length:
            chunk = client_socket.recv(4096)
            if not chunk:
                break
            data += chunk
            print(
                f'Received {len(chunk)} bytes, total received {len(data)} bytes')
        parsed_data = parse_qs(data.decode())
        print(f'Received data from {client_address}: {parsed_data}')

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


def start_server(port):
    # Create a socket and bind it to the specified port
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('localhost', port))
    server_socket.listen()

    print(f'Server listening on port {port}')

    # Handle incoming connections
    while True:
        # Accept a client connection
        client_socket, client_address = server_socket.accept()
        print(f'Accepted connection from {client_address}')

        # Create a new thread to handle the request
        thread = threading.Thread(
            target=handle_request, args=(client_socket, client_address))
        thread.start()


if __name__ == '__main__':
    start_server(8080)
