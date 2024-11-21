/* message_types.h */

#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <stdint.h>

// Define message types
#define MESSAGE_TYPE_STATUS_UPDATE 0
#define MESSAGE_TYPE_REGISTER 1
#define MESSAGE_TYPE_UPDATE 2
#define MESSAGE_TYPE_STATUS_RESPONSE 3



/*
typedef enum {
    MESSAGE_TYPE_CONTROL_COMMAND,
    MESSAGE_TYPE_NEW_GROW_CYCLE,
    MESSAGE_TYPE_TIME_SYNC,
    MESSAGE_TYPE_STATUS_REQUEST,
    MESSAGE_TYPE_STATUS_RESPONSE,
    MESSAGE_TYPE_ERASE_EEPROM,
    // Weitere Nachrichtentypen bei Bedarf hinzufügen
} MessageType;
*/

typedef struct {
    uint8_t message_type;
    uint8_t device;
    uint8_t target;
    uint8_t action;
    uint8_t value;
    char json_payload[512]; // Kann bei Bedarf angepasst werden
} MessageForWebSocket;


#define MESSAGE_POOL_SIZE 10



#endif // MESSAGE_TYPES_H
