import socket

def start_server():
    # Erstelle einen TCP/IP-Socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # Binde den Socket an die Adresse und den Port
    server_address = ('0.0.0.0', 8085)  # Hört auf allen Netzwerk-Interfaces auf Port 8080
    server_socket.bind(server_address)
    
    # Lausche auf eingehende Verbindungen
    server_socket.listen(5)
    print("Server is listening on port 8085...")

    while True:
        # Warte auf eine Verbindung
        client_socket, client_address = server_socket.accept()
        try:
            print(f"Connection from {client_address}")

            while True:
                # Empfange die Daten
                data = client_socket.recv(1024)
                if data:
                    print(f"Received: {data.decode()}")
                    response = "Data received"
                    client_socket.sendall(response.encode())
                else:
                    break
        finally:
            # Schließe die Verbindung
            client_socket.close()

if __name__ == "__main__":
    start_server()
