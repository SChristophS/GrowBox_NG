/*
 * helper_websocket.h
 *
 *  Created on: Jul 29, 2024
 *      Author: chris
 */

#ifndef INC_HELPER_WEBSOCKET_H_
#define INC_HELPER_WEBSOCKET_H_




void add_message_to_websocket_queue(uint8_t message_type, uint8_t device, uint8_t target, uint8_t action, uint16_t value);

const char* message_type_to_string(uint8_t message_type);
const char* device_to_string(uint8_t device);
const char* target_to_string(uint8_t target);
const char* action_to_string(uint8_t action);
#endif /* INC_HELPER_WEBSOCKET_H_ */
