import socket
import threading
import json

def client_handler(client_socket):
    try:
        while True:
            message = client_socket.recv(1024)
            if not message:
                break
            message = message.decode('utf-8').strip()
            
            if not message:  # Überspringe leere Nachrichten
                print("Empty or whitespace-only message received")
                continue            
                            
            if message:  # Prüfe, ob die Nachricht nicht leer ist
                print(f"Received: {message}")
                # Versuche, die Nachricht als JSON zu parsen
                try:
                    message_data = json.loads(message)
                    chipId = message_data.get("chipId")
                    actual_message = message_data.get("message")
                    print(f"ChipID: {chipId}, Message: {actual_message}")
                    # Verarbeite die Nachricht hier weiter basierend auf ChipID
                except json.JSONDecodeError as e:
                    print("Received non-JSON message:", message)
            else:
                print("Empty message received")
    except ConnectionResetError:
        print("Connection reset by peer")
    finally:
        client_socket.close()
        print("Connection closed")


def start_server(host='0.0.0.0', port=8085):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen()

    print(f"Server is listening on {host}:{port}")

    while True:
        client_socket, addr = server_socket.accept()
        print(f"Connection from {addr} has been established.")
        threading.Thread(target=client_handler, args=(client_socket,)).start()


if __name__ == '__main__':
    start_server()
