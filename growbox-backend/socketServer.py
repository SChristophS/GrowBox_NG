import socket
import paho.mqtt.client as mqtt

# Server IP und Port
#HOST = '192.168.178.72'
HOST = '127.0.0.1'

PORT = 8085

# MQTT Broker Einstellungen
MQTT_BROKER = "192.168.178.25"
MQTT_PORT = 49154
#MQTT_TOPIC = "Growbox/topic"
MQTT_TOPIC = "Growbox"
MQTT_CLIENT_ID = "GrowServer_1"
MQTT_USERNAME = "christoph"
MQTT_PASSWORD = "Aprikose99"


# Funktion zur Verarbeitung eingehender MQTT-Nachrichten
def on_mqtt_message(client, userdata, message):
    print(f"MQTT Nachricht erhalten: {message.payload.decode()}")

# Funktion zur Verarbeitung der Verbindung zum Broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Erfolgreich mit dem MQTT-Broker verbunden")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Fehler beim Verbinden mit dem MQTT-Broker: {rc}")

# Funktion zum Starten des MQTT-Clients
def start_mqtt_client():
    client = mqtt.Client(MQTT_CLIENT_ID)
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_mqtt_message

    print("Versuche, eine Verbindung zum MQTT-Broker herzustellen...")
    client.connect(MQTT_BROKER, MQTT_PORT)
    client.loop_start()

    return client

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
    # Starte den MQTT-Client
    mqtt_client = start_mqtt_client()

    # Starte den Socket-Server
    start_server()
