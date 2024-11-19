#include "cmsis_os.h"
#include <stdio.h>
#include "task_network.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <string.h>
#include "globals.h"
#include "helper_websocket.h"
#include "logger.h"

void send_status_update(MessageType message_type, uint8_t device, uint8_t value) {

	LOG_DEBUG("send_status_update: Call with following parameters message_type = %d, device = %d, value = %d", message_type, device, value);
    MessageForWebSocket msg;
    memset(&msg, 0, sizeof(msg)); // Struktur initialisieren

    LOG_DEBUG("send_status_update: Size of MessageForWebSocket: %lu Bytes", (unsigned long)sizeof(MessageForWebSocket));
    LOG_DEBUG("send_status_update: Parameter message_type = %d, device = %d, value = %d", message_type, device, value);

    msg.message_type = message_type;
    msg.device = device;
    msg.value = value;

    // Sicherstellen, dass `json_payload` terminiert ist
    msg.json_payload[0] = '\0';

    LOG_DEBUG("send_status_update: Vor dem Hinzufügen zur Warteschlange: message_type = %d, device = %d, value = %d", msg.message_type, msg.device, msg.value);
    add_message_to_websocket_queue(msg.message_type, msg.device, 0, 0, msg.value);
    LOG_DEBUG("send_status_update: Nachricht zur WebSocket-Warteschlange hinzugefügt");
}

const char* CommandTypeToString(HardwareCommandType commandType)
{
    switch (commandType)
    {
        case COMMAND_CONTROL_PUMP:
            return "COMMAND_CONTROL_PUMP";
        case COMMAND_CONTROL_LIGHT:
            return "COMMAND_CONTROL_LIGHT";
        default:
            LOG_WARN("CommandTypeToString: Received unknown commandType %d", commandType);
            return "UNKNOWN_COMMAND";
    }
}

const char* message_type_to_string(uint8_t message_type) {
    switch (message_type) {
    	case MESSAGE_TYPE_STATUS_UPDATE:
    		return "status_update";
        case MESSAGE_TYPE_REGISTER:
            return "register";
        case MESSAGE_TYPE_UPDATE:
            return "controller_update";
        default:
            LOG_WARN("message_type_to_string: Received unknown message_type %d", message_type);
            return "unknown";
    }
}

const char* device_to_string(uint8_t device) {
    switch (device) {
        case DEVICE_CONTROLLER:
            return "controller";
        case DEVICE_FRONTEND:
            return "frontend";
        default:
            LOG_WARN("device_to_string: Received unknown device %d", device);
            return "unknown";
    }
}

const char* target_to_string(uint8_t target) {
    switch (target) {
        case TARGET_WATER_LEVEL:
            return "wasserbeckenZustand";
        case TARGET_LIGHT_INTENSITY:
            return "lightIntensity";
        case TARGET_READYFORAUTORUN:
            return "readyForAutoRun";
        case TARGET_PUMPE_ZULAUF:
            return "pumpeZulauf";
        case TARGET_PUMPE_ABLAUF:
            return "pumpeAblauf";
        case TARGET_SENSOR_VOLL:
            return "sensorVoll";
        case TARGET_SENSOR_LEER:
            return "sensorLeer";
        default:
            LOG_WARN("target_to_string: Received unknown target %d", target);
            return "unknown";
    }
}

const char* action_to_string(uint8_t action) {
    switch (action) {
        case ACTION_SET:
            return "set";
        case ACTION_UPDATE:
            return "update";
        default:
            LOG_WARN("action_to_string: Received unknown action %d", action);
            return "unknown";
    }
}

void add_message_to_websocket_queue(uint8_t message_type, uint8_t device, uint8_t target, uint8_t action, uint16_t value) {
    MessageForWebSocket msg;
    memset(&msg, 0, sizeof(msg)); // Struktur initialisieren

    msg.message_type = message_type;
    msg.device = device;
    msg.target = target;
    msg.action = action;
    msg.value = value;

    // Sicherstellen, dass `json_payload` terminiert ist
    msg.json_payload[0] = '\0';

    LOG_DEBUG("add_message_to_websocket_queue: Preparing message with type=%d, device=%d, target=%d, action=%d, value=%u",
              msg.message_type, msg.device, msg.target, msg.action, msg.value);

    if (osMessageQueuePut(xWebSocketQueueHandle, &msg, 0, 0) != osOK) {
        LOG_ERROR("add_message_to_websocket_queue: Failed to send message to WebSocketQueue");
    } else {
        LOG_INFO("add_message_to_websocket_queue: New entry in WebSocketQueue: message_type=%d, device=%d, target=%d, action=%d, value=%u",
                 msg.message_type, msg.device, msg.target, msg.action, msg.value);
    }
}
