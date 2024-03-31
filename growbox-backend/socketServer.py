import asyncio
import websockets
import json
from typing import Dict, Optional, Tuple

ClientInfo = Tuple[str, str]
ConnectedClients = Dict[str, websockets.WebSocketServerProtocol]

connected_clients: ConnectedClients = {}


async def handler(websocket: websockets.WebSocketServerProtocol, path: str):
    global connected_clients
    client_key: Optional[str] = None

    async for message in websocket:
        # Weiterleiten der Nachricht an andere Clients
        for target_key, target_websocket in connected_clients.items():
            if target_key != client_key:
                print(f"Forwarding message: {message}")
                await target_websocket.send(message)

        try:
            print(f"Received raw message: {message}")
            data = json.loads(message)

            # Überprüfen und Verarbeiten der Registrierungsnachricht
            if data.get("message") == "register":
                chipId = data.get("chipId", "000000")
                device_type = data.get("device", "unknown")
                client_key = f"{chipId}-{device_type}"

                if client_key not in connected_clients:
                    connected_clients[client_key] = websocket
                    print(f"Registered: {device_type} with chipId {chipId} added.")
                else:
                    print(f"Duplicate: {device_type} with chipId {chipId} already connected.")
                continue  # Weiter mit der nächsten Nachricht

        except json.JSONDecodeError:
            print(f"Invalid JSON: {message}")
            continue
        except Exception as e:
            print(f"Unexpected error: {str(e)}")
            continue




    # Verbindung wurde geschlossen
    if client_key and client_key in connected_clients:
        del connected_clients[client_key]
        print(f"Disconnected: {client_key} removed.")


async def main():
    async with websockets.serve(handler, "0.0.0.0", 8085):
        print("WebSocket server started on ws://0.0.0.0:8085")
        await asyncio.Future()  # Runs forever


if __name__ == "__main__":
    asyncio.run(main())
