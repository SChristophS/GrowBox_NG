import json
import logging
from datetime import datetime
import pytz

#cd dateutil_env/
# source bin/activate
#
#
#
local_timezone = pytz.timezone('Europe/Berlin')
#current_time = datetime.now(local_timezone).strftime('%Y-%m-%dT%H:%M:%S%z')

# Initialisiere Logger
logging.basicConfig(level=logging.DEBUG)  # Ändern Sie das Level zu DEBUG, um alle Nachrichten zu sehen
logger = logging.getLogger(__name__)

from websocket_server import WebsocketServer


# Constants
PORT = 8085

# Mappings
frontend_map = {}   # client_id -> list of chip_ids
controller_map = {} # client_id -> chip_id

def handle_erase_eeprom_message(client, server, message_data):
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')
    
    if not target_UUID or not device:
        logger.error("Invalid EraseEEPROM message received: missing target_UUID or device")
        return
    
    # Erstellen der Nachricht, die an den Controller weitergeleitet wird
    message_to_controller = {
        "target_UUID": target_UUID,
        "device": device,
        "message_type": "EraseEEPROM",
        "payload": {}
    }
    
    logger.debug(f"EraseEEPROM received for target_UUID {target_UUID}: {message_to_controller}")
    forward_message_to_controller(server, target_UUID, message_to_controller)


def handle_time_sync_message(client, server, message_data):
    target_UUID = message_data.get('target_UUID')
    current_time = message_data.get('current_time')
    device = message_data.get('device')
    
    if not target_UUID or not current_time or not device:
        logger.error("Invalid TimeSync message received: missing target_UUID, current_time, or device")
        return
    
    # Erstellen der Nachricht, die an den Controller weitergeleitet wird
    message_to_controller = {
        "target_UUID": target_UUID,
        "current_time": current_time,
        "device": device,
        "message_type": "TimeSync",
        "payload": {
            "current_time": current_time
        }
    }
    
    logger.debug(f"TimeSync received for target_UUID {target_UUID}: {message_to_controller}")
    forward_message_to_controller(server, target_UUID, message_to_controller)

def new_client(client, server):
    logger.info(f"New client connected: {client['id']}")

def client_left(client, server):
    logger.info(f"Client disconnected: {client['id']}")
    frontend_map.pop(client['id'], None)
    controller_map.pop(client['id'], None)

def handle_status_update_message(client, server, message_data):
    uid = message_data.get('UID')
    device = message_data.get('device')
    changed_value = message_data.get('changedValue')
    value = message_data.get('value')

    if not uid or not device or changed_value is None or value is None:
        logger.error("Invalid status_update message received: missing UID, device, changedValue, or value")
        return

    message_to_frontend = {
        "message_type": "status_update",
        "UID": uid,
        "device": device,
        "changedValue": changed_value,
        "value": value
    }

    # Nachricht an relevante Frontend-Clients weiterleiten
    forward_message_to_frontends(server, uid, message_to_frontend)

def message_received(client, server, message):
    try:
        if len(message) > 5000:
            message = message[:5000] + '... [Message truncated]'

        if message == "Ping":
            logger.debug("Received Ping message, ignoring.")
            return

        logger.debug(f"Message from {client['id']}: {message}")
        message_data = json.loads(message)
        message_type = message_data.get('message_type')

        if not message_type:
            logger.error("Message type missing")
            return

        # Debug-Logs hinzufügen
        #logger.debug(f"Received message_type: {repr(message_type)}")
        #logger.debug(f"Available message handlers: {list(MESSAGE_HANDLERS.keys())}")

        handler = MESSAGE_HANDLERS.get(message_type)
        if handler:
            handler(client, server, message_data)
        else:
            logger.warning(f"Unhandled message type: {message_type}")

    except json.JSONDecodeError as e:
        logger.error(f"Invalid JSON message received: {e}")
    except Exception as e:
        logger.exception(f"Error handling message: {e}")


def handle_register_message(client, server, message_data):
    device = message_data.get('device')
    if not device:
        logger.error("Device is missing in the message")
        return

    logger.info(f"Device: {device}")

    if device.lower() == 'controller':
        client_id = message_data.get('UID') or message_data.get('uid')
        if not client_id:
            logger.error("UID is missing in the message")
            return

        logger.info(f"Detected Controller")
        controller_map[client['id']] = client_id
        logger.debug(f"Controller map updated: {controller_map}")
        logger.info(f"Controller client({client['id']}) registered with UID: {client_id}")

        # Benachrichtige Frontend-Clients, dass dieser Controller verfügbar ist
        notify_frontend_clients(server, client_id)



        # Angenommen, deine lokale Zeitzone ist Europa/Berlin
        local_timezone = pytz.timezone('Europe/Berlin')
        current_time = datetime.now(local_timezone).strftime('%Y-%m-%dT%H:%M:%S%z')  # ISO8601 Format mit Zeitzone

        time_sync_message = {
            "target_UUID": client_id,
            "device": "server",
            "message_type": "TimeSync",
            "payload": {
                "current_time": current_time
            }
        }

        logger.debug(f"Sending TimeSync message to controller {client_id}: {time_sync_message}")
        server.send_message(client, json.dumps(time_sync_message))
        logger.info(f"TimeSync message sent to controller {client_id}")

    elif device.lower() == 'frontend':
        logger.info(f"Detected Frontend")

        # Extrahiere chipIDs
        chip_ids = message_data.get('chipIds', [])
        if not chip_ids:
            logger.error("chipIds are missing in the message")
            return

        frontend_map[client['id']] = chip_ids
        logger.debug(f"Frontend map updated: {frontend_map}")
        logger.info(f"Frontend client({client['id']}) registered with chipIDs: {chip_ids}")

        controllers = {}
        for chip_id in chip_ids:
            controllers[chip_id] = [cid for cid, id in controller_map.items() if id == chip_id]

        logger.debug(f"Controllers for chipIDs {chip_ids}: {controllers}")
        logger.debug(f"Current controller map: {controller_map}")

        confirmation_message = {
            "message_type": "register_confirmed",
            "controllers": controllers
        }

        server.send_message(client, json.dumps(confirmation_message))
        logger.debug(f"Confirmation message sent to Frontend client({client['id']}): {confirmation_message}")

        logger.debug(f"Device: {device}, Chip IDs: {chip_ids}")

    else:
        logger.error("Device is unknown")

def handle_control_command_message(client, server, message_data):
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')
    payload = message_data.get('payload')

    if not target_UUID or not device or not payload:
        logger.error("Invalid ControlCommand message received: missing target_UUID, device, or payload")
        return

    # Der Payload sollte eine Liste von Befehlen enthalten
    commands = payload.get('commands')
    if not commands or not isinstance(commands, list):
        logger.error("Invalid payload in ControlCommand message: 'commands' is missing or not a list")
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

    logger.debug(f"ControlCommand received for target_UUID {target_UUID}: {message_to_controller}")
    forward_message_to_controller(server, target_UUID, message_to_controller)

def handle_new_grow_cycle_message(client, server, message_data):
    # Ähnlich wie bei ControlCommand, Nachricht an den Controller weiterleiten
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')
    payload = message_data.get('payload')

    if not target_UUID or not device or not payload:
        logger.error("Invalid newGrowCycle message received: missing target_UUID, device, or payload")
        return

    message_to_controller = message_data  # Angenommen, das Nachrichtenformat ist korrekt

    logger.debug(f"newGrowCycle message received for target_UUID {target_UUID}")
    forward_message_to_controller(server, target_UUID, message_to_controller)

def handle_controller_state_request_message(client, server, message_data):
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')

    if not target_UUID or not device:
        logger.error("Invalid ControllerStateRequest message received: missing target_UUID or device")
        return

    # Nachricht an den Controller weiterleiten
    message_to_controller = message_data  # Angenommen, das Nachrichtenformat ist korrekt
    logger.debug(f"ControllerStateRequest received for target_UUID {target_UUID}")
    forward_message_to_controller(server, target_UUID, message_to_controller)

def handle_grow_cycle_config_request_message(client, server, message_data):
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')

    if not target_UUID or not device:
        logger.error("Invalid GrowCycleConfigRequest message received: missing target_UUID or device")
        return

    # Nachricht an den Controller weiterleiten
    message_to_controller = message_data  # Angenommen, das Nachrichtenformat ist korrekt
    logger.debug(f"GrowCycleConfigRequest received for target_UUID {target_UUID}")
    forward_message_to_controller(server, target_UUID, message_to_controller)

def handle_status_request_message(client, server, message_data):
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')
    payload = message_data.get('payload')

    if not target_UUID or not device or not payload:
        logger.error("Invalid StatusRequest message received: missing target_UUID, device, or payload")
        return

    # Nachricht an den Controller weiterleiten
    message_to_controller = message_data  # Angenommen, das Nachrichtenformat ist korrekt
    logger.debug(f"StatusRequest received for target_UUID {target_UUID}")
    forward_message_to_controller(server, target_UUID, message_to_controller)

def handle_controller_state_response_message(client, server, message_data):
    uid = message_data.get('UID')
    if not uid:
        logger.error("Invalid ControllerStateResponse message received: missing UID")
        return

    logger.debug(f"ControllerStateResponse received from UID {uid}")
    # Nachricht an relevante Frontend-Clients weiterleiten
    forward_message_to_frontends(server, uid, message_data)

def handle_status_response_message(client, server, message_data):
    uid = message_data.get('UID')
    if not uid:
        logger.error("Invalid StatusResponse message received: missing UID")
        return

    logger.debug(f"StatusResponse received from UID {uid}")
    # Nachricht an relevante Frontend-Clients weiterleiten
    forward_message_to_frontends(server, uid, message_data)

def handle_grow_cycle_config_response_message(client, server, message_data):
    uid = message_data.get('UID')
    if not uid:
        logger.error("Invalid GrowCycleConfigResponse message received: missing UID")
        return

    logger.debug(f"GrowCycleConfigResponse received from UID {uid}")
    # Nachricht an relevante Frontend-Clients weiterleiten
    forward_message_to_frontends(server, uid, message_data)

def forward_message_to_controller(server, chip_id, message_data):
    controller_to_notify = [client_id for client_id, id in controller_map.items() if id == chip_id]
    message = json.dumps(message_data)
    logger.debug(f"Forwarding message to controller: {controller_to_notify}")
    for client_id in controller_to_notify:
        client = next((c for c in server.clients if c['id'] == client_id), None)
        if client:
            server.send_message(client, message)
            logger.debug(f"Message sent to controller client {client_id}: {message}")
        else:
            logger.debug(f"Controller client {client_id} not connected")

def forward_message_to_frontends(server, chip_id, message_data):
    frontends_to_notify = [client_id for client_id, chip_ids in frontend_map.items() if chip_id in chip_ids]
    message = json.dumps(message_data)
    logger.debug(f"Forwarding message to frontends: {frontends_to_notify}")
    for client_id in frontends_to_notify:
        client = next((c for c in server.clients if c['id'] == client_id), None)
        if client:
            server.send_message(client, message)
            logger.debug(f"Message sent to frontend client {client_id}: {message}")
        else:
            logger.debug(f"Frontend client {client_id} not connected")

def notify_frontend_clients(server, client_id):
    frontend_clients_to_notify = []
    for frontend_client_id, chip_ids in frontend_map.items():
        if client_id in chip_ids:
            frontend_clients_to_notify.append(frontend_client_id)

    logger.debug(f"Frontend clients to notify for chipID {client_id}: {frontend_clients_to_notify}")

    for frontend_client_id in frontend_clients_to_notify:
        client = next((c for c in server.clients if c['id'] == frontend_client_id), None)
        if client:
            message_to_frontend = {
                "message_type": "controller_update",
                "chipID": client_id
            }
            server.send_message(client, json.dumps(message_to_frontend))
            logger.debug(f"Message sent to frontend client {frontend_client_id}: {message_to_frontend}")
        else:
            logger.debug(f"Frontend client {frontend_client_id} not connected")

def handle_automatic_mode_request_message(client, server, message_data):
    target_UUID = message_data.get('target_UUID')
    device = message_data.get('device')

    if not target_UUID or not device:
        logger.error("Invalid AutomaticModeRequest message received: missing target_UUID or device")
        return

    # Nachricht an den Controller weiterleiten
    message_to_controller = message_data
    logger.debug(f"AutomaticModeRequest received for target_UUID {target_UUID}")
    forward_message_to_controller(server, target_UUID, message_to_controller)

def handle_automatic_mode_response_message(client, server, message_data):
    uid = message_data.get('UID')
    if not uid:
        logger.error("Invalid AutomaticModeResponse message received: missing UID")
        return

    logger.debug(f"AutomaticModeResponse received from UID {uid}")
    # Nachricht an relevante Frontend-Clients weiterleiten
    forward_message_to_frontends(server, uid, message_data)
    
# Mapping von Nachrichtentypen zu Handler-Funktionen
MESSAGE_HANDLERS = {
    'register': handle_register_message,
    'newGrowCycle': handle_new_grow_cycle_message,
    'ControlCommand': handle_control_command_message,
    'StatusUpdate': handle_status_update_message,
    'TimeSync': handle_time_sync_message,
    'EraseEEPROM': handle_erase_eeprom_message,
    'ControllerStateRequest': handle_controller_state_request_message,
    'GrowCycleConfigRequest': handle_grow_cycle_config_request_message,
    'StatusRequest': handle_status_request_message,
    'ControllerStateResponse': handle_controller_state_response_message,
    'GrowCycleConfigResponse': handle_grow_cycle_config_response_message,
    'StatusResponse': handle_status_response_message,
    'AutomaticModeRequest': handle_automatic_mode_request_message,
    'AutomaticModeResponse': handle_automatic_mode_response_message,
    # Weitere Nachrichtentypen...
}



# Starten des Servers
if __name__ == "__main__":
    server = WebsocketServer(port=PORT)
    server.set_fn_new_client(new_client)
    server.set_fn_client_left(client_left)
    server.set_fn_message_received(message_received)
    logger.info(f"Server started on port {PORT}")
    server.run_forever()
