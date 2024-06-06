import socket

# Server IP und Port
HOST = '192.168.178.72'
PORT = 8085

def start_server():
    # Erstellt ein Socket-Objekt
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        # Bindet das Socket an die angegebene IP und den Port
        server_socket.bind((HOST, PORT))
        # Wartet auf eingehende Verbindungen
        server_socket.listen()
        print(f'Server gestartet. Wartet auf Verbindungen bei {HOST}:{PORT}...')

        # Akzeptiert eine eingehende Verbindung
        conn, addr = server_socket.accept()
        with conn:
            print(f'Verbunden mit {addr}')
            while True:
                # Empfängt Daten vom Client
                data = conn.recv(1024)
                if not data:
                    break
                print(f'Erhalten: {data.decode()}')
                # Sendet die empfangenen Daten zurück
                conn.sendall(data)

if __name__ == "__main__":
    start_server()
