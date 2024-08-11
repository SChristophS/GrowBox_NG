/*
 * helper_websocket.c
 *
 *  Created on: Jul 29, 2024
 *      Author: chris
 */

#include "cmsis_os.h"
#include <stdio.h>
#include "task_network.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>
#include <string.h>
#include "task_network.h"
#include "globals.h"


const char* message_type_to_string(uint8_t message_type) {
    switch (message_type) {
        case MESSAGE_TYPE_REGISTER: return "register";
        case MESSAGE_TYPE_UPDATE: return "controller_update";
        default: return "unknown";
    }
}

const char* device_to_string(uint8_t device) {
    switch (device) {
        case DEVICE_CONTROLLER: return "controller";
        case DEVICE_FRONTEND: return "frontend";
        default: return "unknown";
    }
}

const char* target_to_string(uint8_t target) {
    switch (target) {
        case TARGET_WATER_LEVEL: return "wasserbeckenZustand";
        case TARGET_LIGHT_INTENSITY: return "lightIntensity";
        case TARGET_READYFORAUTORUN: return "readyForAutoRun";
        case TARGET_PUMPE_ZULAUF: return "pumpeZulauf";
        case TARGET_PUMPE_ABLAUF: return "pumpeAblauf";
        case TARGET_SENSOR_VOLL: return "sensorVoll";
        case TARGET_SENSOR_LEER: return "sensorLeer";
        default: return "unknown";
    }
}

const char* action_to_string(uint8_t action) {
    switch (action) {
        case ACTION_SET: return "set";
        case ACTION_UPDATE: return "update";
        default: return "unknown";
    }
}


void add_message_to_websocket_queue(uint8_t message_type, uint8_t device, uint8_t target, uint8_t action, uint16_t value) {
    MessageForWebSocket msg;

    msg.message_type = message_type;
    msg.device = device;
    msg.target = target;
    msg.action = action;
    msg.value = value;

    printf("helper_websocket.c: I add the target %d with the action %d and value %u\r\n", msg.target, msg.action, msg.value);

    if (osMessageQueuePut(xWebSocketQueueHandle, &msg, 0, 0) != osOK) {
        printf("helper_websocket.c: Failed to send message to WebSocketQueue.\r\n");
    } else {
        printf("helper_websocket.c: new entry in WebSocketQueue: message_type=%d, device=%d, target=%d, action=%d, value=%u\r\n",
               msg.message_type, msg.device, msg.target, msg.action, msg.value);
    }
}

