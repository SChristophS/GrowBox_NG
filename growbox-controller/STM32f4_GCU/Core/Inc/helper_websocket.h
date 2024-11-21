/*
 * helper_websocket.h
 *
 *  Created on: Jul 29, 2024
 *      Author: chris
 */

#ifndef INC_HELPER_WEBSOCKET_H_
#define INC_HELPER_WEBSOCKET_H_

#include <stdint.h>
#include "message_types.h"
#include "globals.h"

void send_status_update(uint8_t message_type, uint8_t device, uint8_t target, uint8_t value);

const char* CommandTypeToString(HardwareCommandType commandType);

void add_message_to_websocket_queue(uint8_t message_type, uint8_t device, uint8_t target, uint8_t action, uint16_t value);

const char* message_type_to_string(uint8_t message_type);
const char* device_to_string(uint8_t device);
const char* target_to_string(uint8_t target);
const char* action_to_string(uint8_t action);


void freeMessage(MessageForWebSocket* msg);
MessageForWebSocket* allocateMessage();

typedef struct {
    uint8_t inUse;
    MessageForWebSocket message;
} MessagePoolItem;

extern MessagePoolItem messagePool[MESSAGE_POOL_SIZE]; // Deklaration mit extern

#endif /* INC_HELPER_WEBSOCKET_H_ */
