/* message_types.h */

#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <stdint.h>

// Define message types
#define MESSAGE_TYPE_STATUS_UPDATE 0
#define MESSAGE_TYPE_REGISTER 1
#define MESSAGE_TYPE_UPDATE 2


// Define devices
#define DEVICE_CONTROLLER 1
#define DEVICE_FRONTEND 2

// Connected Hardware
#define DEVICE_LIGHT 1
#define DEVICE_PUMP_ZULAUF 2
#define DEVICE_PUMP_ABLAUF 3
#define DEVICE_SENSOR_OBEN 4
#define DEVICE_SENSOR_UNTEN 5


typedef enum {
    MESSAGE_TYPE_CONTROL_COMMAND,
    MESSAGE_TYPE_NEW_GROW_CYCLE,
    MESSAGE_TYPE_TIME_SYNC,
    MESSAGE_TYPE_STATUS_REQUEST,
    MESSAGE_TYPE_STATUS_RESPONSE,
    MESSAGE_TYPE_ERASE_EEPROM,
    // Weitere Nachrichtentypen bei Bedarf hinzufügen
} MessageType;

typedef struct {
    MessageType message_type;
    uint8_t device;
    uint8_t target;
    uint8_t action;
    uint8_t value;
    char json_payload[512]; // Für JSON-Nachrichten
} MessageForWebSocket;

#endif // MESSAGE_TYPES_H
