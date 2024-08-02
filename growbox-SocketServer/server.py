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

# Called when a client sends a message
def message_received(client, server, message):
    print("MessageReceived")
    try:
        if len(message) > 5000:
            message = message[:5000] + '.MSG TOO LONG.'

        if message == "Ping":
            print("Received a Ping message, not forwarding.")
            return

        print(f"Client({client['id']}) said: {message}")

        try:
            message_data = json.loads(message)
            
            # Handle all kind of register messages
            if message_data.get('message_type') == 'register':
                print(f"Client({client['id']}) sent a register-message")
                
                # check the sending device
                device = message_data.get('device')
                print(f"Device: {device}")
                
                if device.lower() == 'controller':  # Make device comparison case-insensitive
                    client_id = message_data.get('UID')
                    print(f"Detected Controller")
                    controller_map[client['id']] = client_id
                    print(f"[DEBUG] Controller map updated: {controller_map}")
                    print(f"Controller client({client['id']}) registered with chipID: {client_id}")
                    
                    # Überprüfen, ob es Frontend-Clients gibt, die auf diesen Controller warten
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
                            print(f"[DEBUG] Client ID {frontend_client_id} not found in server clients")
                
                elif device.lower() == 'frontend':  # Make device comparison case-insensitive
                    print(f"Detected Frontend")
                    
                    # extract chipIDs
                    chip_ids = message_data.get('chipIds', [])

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
                

            elif message_data.get('action') == 'update' and message_data.get('device').lower() == 'controller':
                chip_id = message_data.get('chipID')
                state_update = message_data.get('state')
                print(f"[DEBUG] State update received for chipID {chip_id}: {state_update}")
                forward_state_update_to_frontends(chip_id, state_update)
            else:
                print(f"[DEBUG] Broadcasting message to all clients: {message}")
                server.send_message_to_all(message)
                
        except json.JSONDecodeError:
            print("Invalid JSON message received")

    except Exception as e:
        print(f"Error handling message: {e}")

# Function to forward state updates to relevant frontends
def forward_state_update_to_frontends(chip_id, state_update):
    frontends_to_notify = [client_id for client_id, chip_ids in frontend_map.items() if chip_id in chip_ids]
    message = json.dumps({"action": "update", "chipID": chip_id, "state": state_update})
    print(f"[DEBUG] Forwarding state update to frontends: {frontends_to_notify}")
    for client_id in frontends_to_notify:
        if any(c['id'] == client_id for c in server.clients):
            server.send_message(next(c for c in server.clients if c['id'] == client_id), message)
            print(f"[DEBUG] State update message sent to frontend client {client_id}: {message}")

PORT = 8085
server = WebsocketServer(port=PORT)
server.set_fn_new_client(new_client)
server.set_fn_client_left(client_left)
server.set_fn_message_received(message_received)
server.run_forever()
