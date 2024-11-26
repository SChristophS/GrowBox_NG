/* helper_websocket.c */
#include "cmsis_os.h"
#include <stdio.h>
#include "task_network.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <string.h>
#include "globals.h"
#include "helper_websocket.h"
#include "logger.h"

MessagePoolItem messagePool[MESSAGE_POOL_SIZE];

// Initialisierung des Nachrichtenpools und des Mutex
void initialize_message_pool() {
    // Initialisieren des Nachrichtenpools
    for (int i = 0; i < MESSAGE_POOL_SIZE; i++) {
        messagePool[i].inUse = false;
        memset(&messagePool[i].message, 0, sizeof(MessageForWebSocket));
    }
}

MessageForWebSocket* allocateMessage() {
    osMutexAcquire(gMessagePoolMutexHandle, osWaitForever);
    for (int i = 0; i < MESSAGE_POOL_SIZE; i++) {
        if (!messagePool[i].inUse) {
            messagePool[i].inUse = 1;
            // Nachricht initialisieren
            memset(&messagePool[i].message, 0, sizeof(MessageForWebSocket));
            osMutexRelease(gMessagePoolMutexHandle);
            return &messagePool[i].message;
        }
    }
    osMutexRelease(gMessagePoolMutexHandle);
    LOG_WARN("allocateMessage: Message pool exhausted");
    return NULL; // Pool erschöpft
}

void freeMessage(MessageForWebSocket* msg) {
    osMutexAcquire(gMessagePoolMutexHandle, osWaitForever);
    for (int i = 0; i < MESSAGE_POOL_SIZE; i++) {
        if (&messagePool[i].message == msg) {
            messagePool[i].inUse = 0;
            // Nachricht löschen (optional)
            memset(&messagePool[i].message, 0, sizeof(MessageForWebSocket));
            osMutexRelease(gMessagePoolMutexHandle);
            return;
        }
    }
    osMutexRelease(gMessagePoolMutexHandle);
    // Fehlerbehandlung, falls erforderlich
}



;

void send_status_update(const char *message_type, uint8_t device, uint8_t target, uint32_t value) {
    LOG_DEBUG("send_status_update: Call with following parameters message_type = %d, device = %d, value = %d", message_type, device, value);

    MessageForWebSocket* msg = allocateMessage();
    if (msg == NULL) {
        LOG_ERROR("send_status_update: Failed to allocate message");
        return;
    }

    msg->message_type = message_type;
    msg->device = device;
    msg->target = target;
    msg->value = value;
    msg->action = 0; // Setzen, falls erforderlich
    msg->json_payload[0] = '\0';

    LOG_DEBUG("send_status_update: Vor dem Hinzufügen zur Warteschlange: message_type = %d, device = %d, value = %d", msg->message_type, msg->device, msg->value);

    // Nachricht zur WebSocket-Queue hinzufügen
    if (osMessageQueuePut(xWebSocketQueueHandle, &msg, 0, 0) != osOK) {
        LOG_ERROR("task_network.c: Failed to send JSON over WebSocket");
        freeMessage(msg); // Nachricht freigeben, falls das Hinzufügen fehlschlägt
    } else {
        LOG_INFO("task_network.c: JSON message added to WebSocketQueue");
    }
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




const char* device_to_string(uint8_t device) {
    switch (device) {
        case DEVICE_CONTROLLER:
            return "controller";
        case DEVICE_FRONTEND:
            return "frontend";
        /*
        case DEVICE_PUMP_ZULAUF:
            return "pump_zulauf";
        case DEVICE_PUMP_ABLAUF:
            return "pump_ablauf";
        case DEVICE_SENSOR_OBEN:
            return "sensorOben";
        case DEVICE_SENSOR_UNTEN:
            return "sensorUnten";
        */
        // Fügen Sie weitere Geräte hinzu, falls erforderlich
        default:
            LOG_WARN("device_to_string: Received unknown device %d", device);
            return "unknown";
    }
}


const char* target_to_string(uint8_t target) {
    switch (target) {
        case TARGET_LIGHT_INTENSITY:
            return "lightIntensity";
        case TARGET_WATER_LEVEL:
            return "waterLevel";
        case TARGET_READYFORAUTORUN:
            return "readForAutorun";
        case TARGET_PUMPE_ZULAUF:
            return "pumpeZulauf";
        case TARGET_PUMPE_ABLAUF:
			return "pumpeAblauf";
        // Fügen Sie weitere Ziele hinzu, falls erforderlich
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



void add_message_to_websocket_queue(const char *message_type, uint8_t device, uint8_t target, uint8_t action, uint16_t value) {
    MessageForWebSocket* msg = allocateMessage();
    if (msg == NULL) {
        LOG_ERROR("add_message_to_websocket_queue: Failed to allocate message");
        return;
    }

    msg->message_type = message_type;
    msg->device = device;
    msg->target = target;
    msg->action = action;
    msg->value = value;
    msg->json_payload[0] = '\0';

    LOG_DEBUG("add_message_to_websocket_queue: Preparing message with type=%d, device=%d, target=%d, action=%d, value=%u",
              msg->message_type, msg->device, msg->target, msg->action, msg->value);

    if (osMessageQueuePut(xWebSocketQueueHandle, &msg, 0, 0) != osOK) {
        LOG_ERROR("add_message_to_websocket_queue: Failed to send message to WebSocketQueue");
        freeMessage(msg);
    } else {
        LOG_INFO("add_message_to_websocket_queue: New entry in WebSocketQueue: message_type=%d, device=%d, target=%d, action=%d, value=%u",
                 msg->message_type, msg->device, msg->target, msg->action, msg->value);
    }
}

