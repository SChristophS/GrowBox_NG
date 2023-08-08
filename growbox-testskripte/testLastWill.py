import paho.mqtt.client as mqtt
import time

broker = "192.168.178.25"
port = 49154
username = "christoph"
password = "Aprikose99"

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    print("Subscribing to growbox/disconnected...")
    client.subscribe("growbox/disconnected")

def on_disconnect(client, userdata, rc):
    print(f"Disconnected with result code {rc}")

def on_message(client, userdata, msg):
    print(f"Received message: {msg.topic} {msg.payload}")

client = mqtt.Client(client_id="PythonClient")

client.username_pw_set(username, password)
client.will_set("growbox/disconnected", payload="PythonClient", qos=0, retain=False)

client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message

print("Connecting to broker...")
client.connect(broker, port, 60)

client.loop_start()

print("Sleeping for 10 seconds...")
time.sleep(10)

print("Disconnecting...")
#client.disconnect()

client.loop_stop()