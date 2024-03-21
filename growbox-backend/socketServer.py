import socket

def start_server(host='0.0.0.0', port=8085):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen()

    print(f"Server is listening on {host}:{port}")

    while True:
        client_socket, addr = server_socket.accept()
        print(f"Connection from {addr} has been established.")
        
        while True:
            try:
                message = client_socket.recv(1024)
                if not message:
                    break
                print(f"Received: {message.decode('utf-8')}")
                # Echo the received message back
                client_socket.send(message)
            except ConnectionResetError:
                break
        
        client_socket.close()
        print(f"Connection with {addr} closed.")

if __name__ == '__main__':
    start_server()