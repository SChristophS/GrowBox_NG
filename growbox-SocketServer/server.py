from websocket_server import WebsocketServer
import json

# Dictionaries to map client IDs to chip IDs
frontend_map = {}
controller_map = {}

# Called for every client connecting (after handshake)
def new_client(client, server):
    print(f"New client connected and was given id {client['id']}")
    print(f"[DEBUG] Current clients: {server.clients}")

# Called for every client disconnecting
def client_left(client, server):
    print(f"Client({client['id']}) disconnected")
    if client['id'] in frontend_map:
        del frontend_map[client['id']]
    elif client['id'] in controller_map:
        del controller_map[client['id']]
    print(f"[DEBUG] Updated clients after disconnect: {server.clients}")

def message_received(client, server, message):
    try:
        if len(message) > 5000:
            message = message[:5000] + '.MSG TOO LONG.'

        if message == "Ping":
            print("Received a Ping message, not forwarding.")
            return

        print(f"Client({client['id']}) said: {message}")

        try:
            print("try to load the json-message")
            message_data = json.loads(message)
            print("Message loaded")

            message_type = message_data.get('message_type')
            if not message_type:
                print("Message type is missing in the message")
                return
            print("Message type is ok")

            # Unterschiedliche Nachrichtentypen behandeln
            if message_type == 'register':
                handle_register_message(client, server, message_data)
            elif message_type == 'newGrowCycle':
                handle_new_grow_cycle_message(client, server, message_data)
            elif message_type == 'ControlCommand':
                handle_control_command_message(client, server, message_data)
            else:
                print(f"[DEBUG] Unsupported message type received: {message_type}")

        except json.JSONDecodeError:
            print("Invalid JSON message received")

    except Exception as e:
        print(f"Error handling message: {e}")


def handle_register_message(client, server, message_data):
    device = message_data.get('device')
    if not device:
        print("Device is missing in the message")
        return

    print(f"Device: {device}")

    if device.lower() == 'controller':  # Make device comparison case-insensitive
        client_id = message_data.get('UID') or message_data.get('uid')
        if not client_id:
            print("UID is missing in the message")
            return

        print(f"Detected Controller")
        controller_map[client['id']] = client_id
        print(f"[DEBUG] Controller map updated: {controller_map}")
        print(f"Controller client({client['id']}) registered with UID: {client_id}")

        # Benachrichtige Frontend-Clients, dass dieser Controller verfügbar ist
        notify_frontend_clients(server, client_id)

    elif device.lower() == 'frontend':  # Make device comparison case-insensitive
        print(f"Detected Frontend")

        # Extrahiere chipIDs
        chip_ids = message_data.get('chipIds', [])
        if not chip_ids:
            print("chipIds are missing in the message")
            return

        frontend_map[client['id']] = chip_ids
        print(f"[DEBUG] Frontend map updated: {frontend_map}")
        print(f"Frontend client({client['id']}) registered with chipIDs: {chip_ids}")

        controllers = {}
        for chip_id in chip_ids:
            controllers[chip_id] = [cid for cid, id in controller_map.items() if id == chip_id]

        print(f"[DEBUG] Controllers for chipIDs {chip_ids}: {controllers}")
        print(f"[DEBUG] Current controller map: {controller_map}")

        confirmation_message = {
            "message_type": "register_confirmed",
            "controllers": controllers
        }

        server.send_message(client, json.dumps(confirmation_message))
        print(f"[DEBUG] Confirmation message sent to Frontend client({client['id']}): {confirmation_message}")

        print(f"[DEBUG] Device: {device}, Chip IDs: {chip_ids}")

    else:
        print("Device is unknown")

def handle_control_command_message(client, server, message_data):
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')
    payload = message_data.get('payload')

    if not target_UUID or not device or not payload:
        print("Invalid ControlCommand message received: missing target_UUID, device, or payload")
        return

    # Der Payload sollte eine Liste von Befehlen enthalten
    commands = payload.get('commands')
    if not commands or not isinstance(commands, list):
        print("Invalid payload in ControlCommand message: 'commands' is missing or not a list")
        return

    # Nachricht vorbereiten, um sie an den Controller weiterzuleiten
    message_to_controller = {
        "target_UUID": target_UUID,
        "current_time": message_data.get('current_time'),
        "device": device,
        "message_type": "ControlCommand",
        "payload": {
            "commands": commands
        }
    }

    print(f"[DEBUG] ControlCommand received for target_UUID {target_UUID}: {message_to_controller}")
    forward_message_to_controller(target_UUID, message_to_controller)

def handle_new_grow_cycle_message(client, server, message_data):
    # Ähnlich wie bei ControlCommand, Nachricht an den Controller weiterleiten
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')
    payload = message_data.get('payload')

    if not target_UUID or not device or not payload:
        print("Invalid newGrowCycle message received: missing target_UUID, device, or payload")
        return

    message_to_controller = message_data  # Angenommen, das Nachrichtenformat ist korrekt

    print(f"[DEBUG] newGrowCycle message received for target_UUID {target_UUID}")
    forward_message_to_controller(target_UUID, message_to_controller)

def forward_message_to_controller(chip_id, message_data):
    controller_to_notify = [client_id for client_id, id in controller_map.items() if id == chip_id]
    message = json.dumps(message_data)
    print(f"[DEBUG] Forwarding message to controller: {controller_to_notify}")
    for client_id in controller_to_notify:
        if any(c['id'] == client_id for c in server.clients):
            server.send_message(next(c for c in server.clients if c['id'] == client_id), message)
            print(f"[DEBUG] Message sent to controller client {client_id}: {message}")
        else:
            print(f"[DEBUG] Controller client {client_id} not connected")

def forward_message_to_frontends(chip_id, message_data):
    frontends_to_notify = [client_id for client_id, chip_ids in frontend_map.items() if chip_id in chip_ids]
    message = json.dumps(message_data)
    print(f"[DEBUG] Forwarding message to frontends: {frontends_to_notify}")
    for client_id in frontends_to_notify:
        if any(c['id'] == client_id for c in server.clients):
            server.send_message(next(c for c in server.clients if c['id'] == client_id), message)
            print(f"[DEBUG] Message sent to frontend client {client_id}: {message}")
        else:
            print(f"[DEBUG] Frontend client {client_id} not connected")

def notify_frontend_clients(server, client_id):
    frontend_clients_to_notify = []
    for frontend_client_id, chip_ids in frontend_map.items():
        if client_id in chip_ids:
            frontend_clients_to_notify.append(frontend_client_id)

    print(f"[DEBUG] Frontend clients to notify for chipID {client_id}: {frontend_clients_to_notify}")

    for frontend_client_id in frontend_clients_to_notify:
        if any(c['id'] == frontend_client_id for c in server.clients):
            message_to_frontend = {
                "message_type": "controller_update",
                "chipID": client_id
            }
            server.send_message(next(c for c in server.clients if c['id'] == frontend_client_id), json.dumps(message_to_frontend))
            print(f"[DEBUG] Message sent to frontend client {frontend_client_id}: {message_to_frontend}")
        else:
            print(f"[DEBUG] Frontend client {frontend_client_id} not connected")


PORT = 8085
server = WebsocketServer(port=PORT)
server.set_fn_new_client(new_client)
server.set_fn_client_left(client_left)
server.set_fn_message_received(message_received)
server.run_forever()
