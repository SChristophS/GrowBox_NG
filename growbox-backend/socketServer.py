import asyncio
import websockets
import json
from typing import Dict, Optional, Tuple

ClientInfo = Tuple[str, str]
ConnectedClients = Dict[str, websockets.WebSocketServerProtocol]

connected_clients: ConnectedClients = {}

async def send_device_list(websocket, chipId):
    # Erstellen einer Liste der registrierten Geräte mit der gleichen chipId
    device_list = [
        key for key in connected_clients if key.startswith(f"{chipId}-")
    ]
    device_list_message = json.dumps({
        "type": "device_list",
        "device_id": chipId,
        "devices": device_list
    })
    await websocket.send(device_list_message)
    print(f"[DEBUG] Sent device list: {device_list_message} to {chipId}")
    
      
async def send_device_list_to_all_with_chipId(chipId):
    device_list = [key for key in connected_clients if key.startswith(f"{chipId}-")]
    device_list_message = json.dumps({
        "type": "device_list",
        "device_id": chipId,
        "devices": device_list
    })

    for client_key, websocket in connected_clients.items():
        if client_key.startswith(f"{chipId}-"):
            await websocket.send(device_list_message)
            print(f"[DEBUG] Sent device list to {client_key} with chipId {chipId}")


async def handler(websocket: websockets.WebSocketServerProtocol, path: str):
    global connected_clients
    client_key: Optional[str] = None

    try:
        async for message in websocket:
            print(f"Received message: {message}")
            data = json.loads(message)

            if data.get("message") == "register":
                chipId = data.get("chipId", "000000")
                device_type = data.get("device", "unknown")
                client_key = f"{chipId}-{device_type}"

                if client_key not in connected_clients:
                    connected_clients[client_key] = websocket
                    print(f"[DEBUG] Registered: {device_type} with chipId {chipId}")
                    await send_device_list_to_all_with_chipId(chipId)
                else:
                    print(f"[DEBUG] Duplicate: {device_type} with chipId {chipId} already connected.")
            
            elif data.get("action") == "send_growplan":
                chipId = data.get("chipId")
                # Stellen Sie sicher, dass `data.get("message")` die Growplan-Daten enthält, die Sie senden möchten
                growplan_data = data.get("message")  # Dies sollte bereits ein Dictionary der Growplan-Daten sein

                # Erstellen Sie die Nachricht mit der neuen Struktur
                new_message_structure = json.dumps({
                    "device": "frontend",
                    "chipId": chipId,
                    "message": growplan_data,  # Da growplan_data bereits ein Dictionary ist, setzen Sie es direkt
                    "action": "new_growplan"
                })

                # Senden der Nachricht an alle verbundenen Controller mit der gleichen chipId
                targets_sent_to = []
                for target_key, target_websocket in connected_clients.items():
                    if target_key.startswith(f"{chipId}-controller"):  # Achten Sie darauf, dass dies mit Ihrer Benennungskonvention übereinstimmt
                        await target_websocket.send(new_message_structure)  # Senden der neu strukturierten Nachricht
                        new_message_structure
                        print(f"[DEBUG] Sent growplan: {new_message_structure}")
                print(f"[DEBUG] Sent growplan to: {targets_sent_to}")
                
    except websockets.exceptions.ConnectionClosedError:
        print("[DEBUG] Connection closed unexpectedly")
    finally:
        if client_key and client_key in connected_clients:
            del connected_clients[client_key]
            print(f"[DEBUG] Disconnected: {client_key} removed.")
            if client_key.split('-', 1)[0] == chipId: # Falls die getrennte Verbindung zu dem Controller gehört, aktualisieren Sie die Geräteliste
                await send_device_list_to_all_with_chipId(chipId)


async def main():
    async with websockets.serve(handler, "0.0.0.0", 8085):
        print("WebSocket server started on ws://0.0.0.0:8085")
        await asyncio.Future()  # Runs forever


if __name__ == "__main__":
    asyncio.run(main())
