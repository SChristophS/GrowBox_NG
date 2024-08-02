// globals.h

#ifndef GLOBALS_H
#define GLOBALS_H

extern char uidStr[25];


// Define message types
#define MESSAGE_TYPE_REGISTER 1
#define MESSAGE_TYPE_UPDATE 2

// Define devices
#define DEVICE_CONTROLLER 1
#define DEVICE_FRONTEND 2

// Define targets
#define TARGET_WATER_LEVEL 1
#define TARGET_LIGHT_INTENSITY 2
#define TARGET_READYFORAUTORUN 3

// Define actions
#define ACTION_SET 1
#define ACTION_UPDATE 2

typedef struct {
    uint8_t message_type;
    uint8_t device;
    uint8_t target;
    uint8_t action;
    uint16_t value;
} MessageForWebSocket;

#endif // GLOBALS_H
