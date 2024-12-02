/* helper_websocket.h */

#ifndef INC_HELPER_WEBSOCKET_H_
#define INC_HELPER_WEBSOCKET_H_

#include <stdint.h>
#include "message_types.h"
#include "globals.h"

void send_status_update(const char *message_type, uint8_t device, uint8_t target, uint32_t value);

const char* CommandTypeToString(HardwareCommandType commandType);

void add_message_to_websocket_queue(const char *message_type, uint8_t device, uint8_t target, uint8_t action, uint16_t value);

const char* message_type_to_string(uint8_t message_type);
const char* device_to_string(uint8_t device);
const char* target_to_string(uint8_t target);
const char* action_to_string(uint8_t action);

void initialize_message_pool();

void send_register_message();
void send_ready_message();

void freeMessage(MessageForWebSocket* msg);
MessageForWebSocket* allocateMessage();

typedef struct {
    uint8_t inUse;
    MessageForWebSocket message;
} MessagePoolItem;

extern MessagePoolItem messagePool[MESSAGE_POOL_SIZE]; // Deklaration mit extern

#endif /* INC_HELPER_WEBSOCKET_H_ */
