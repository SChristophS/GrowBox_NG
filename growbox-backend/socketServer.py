import asyncio
import websockets
import json

connected_clients = {}

async def handler(websocket, path):
    global connected_clients
    client_info = None  # Speichert chipId und device_type des aktuellen WebSockets
    try:
        async for message in websocket:
            print(f"Raw message: {message}")
            try:
                data = json.loads(message)
            except json.JSONDecodeError:
                print(f"Invalid JSON: {message}")
                continue

            chipId = data.get("chipId")
            device_type = data.get("device")

            if chipId and device_type:
                client_key = f"{chipId}-{device_type}"
                if client_key not in connected_clients:  # Überprüfe, ob der Client bereits verbunden ist
                    connected_clients[client_key] = websocket
                    client_info = (chipId, device_type)  # Speichere Client-Info für Disconnect-Handling
                    print(f"{device_type} with chipId {chipId} connected and added to connected clients.")
                else:
                    print(f"{device_type} with chipId {chipId} is already connected.")

            # Weiterleitung der Nachricht an verbundene Clients
            target_device = "controller" if device_type == "Frontend" else "Frontend"
            target_key = f"{chipId}-{target_device}"
            if target_key in connected_clients:
                # Senden der Nachricht an das verbundene Gerät
                await connected_clients[target_key].send(message)
                print(f"Message from {device_type} forwarded to {target_device}.")

    except websockets.exceptions.ConnectionClosed:
        print("A connection was closed")
        if client_info:
            client_key = f"{client_info[0]}-{client_info[1]}"
            if client_key in connected_clients:
                connected_clients.pop(client_key, None)  # Entferne die geschlossene Verbindung
                print(f"{client_info[1]} with chipId {client_info[0]} disconnected and removed from connected clients.")

async def main():
    async with websockets.serve(handler, "0.0.0.0", 8085):
        await asyncio.Future()  # Runs forever

if __name__ == "__main__":
    asyncio.run(main())
