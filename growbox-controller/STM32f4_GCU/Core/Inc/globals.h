#ifndef GLOBALS_H
#define GLOBALS_H

#include "controller_state.h"
#include "schedules.h"
#include "cmsis_os.h" // Für RTOS-Typen

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
#define TARGET_PUMPE_ZULAUF 4
#define TARGET_PUMPE_ABLAUF 5
#define TARGET_SENSOR_VOLL 6
#define TARGET_SENSOR_LEER 7

// Define actions
#define ACTION_SET 1
#define ACTION_UPDATE 2

// Event-Bits für Benachrichtigung
#define WATER_SENSOR_VALUES_CHANGED_BIT (1 << 0)
#define PUMP_ZULAUF_STATE_CHANGED_BIT (1 << 1)
#define PUMP_ABLAUF_STATE_CHANGED_BIT (1 << 2)
#define SENSOR_VOLL_STATE_CHANGED_BIT (1 << 3)
#define SENSOR_LEER_STATE_CHANGED_BIT (1 << 4)
#define LIGHT_INTENSITY_CHANGED_BIT (1 << 5)
#define READY_FOR_AUTORUN_STATE_CHANGED_BIT (1 << 6)
#define WATER_STATE_CHANGED_BIT (1 << 7)
#define NEW_GROW_CYCLE_CONFIG_AVAILABLE_LIGHT  (1 << 8)
#define NEW_GROW_CYCLE_CONFIG_AVAILABLE_WATER  (1 << 9)
#define INITIALIZATION_COMPLETE          (1 << 10)


/* Netzwerk-Einstellungen */
#define MY_IP          {192, 168, 178, 100}
#define SUBNET_MASK    {255, 255, 255, 0}
#define GATEWAY        {192, 168, 178, 1}
#define DNS_SERVER     {8, 8, 8, 8}
#define MAC_ADDRESS    {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef}
#define LOCAL_PORT     50000

// Globale Variablen
extern ControllerState gControllerState;
extern GrowCycleConfig gGrowCycleConfig;

/* TODO the name should be gAutomaticMode */
extern bool automaticMode;

extern char uidStr[25];

// Mutexe und Event-Gruppen
extern osMutexId_t gControllerStateMutexHandle;
extern osMutexId_t gGrowCycleConfigMutexHandle;
extern osMutexId_t gAutomaticModeHandle;
extern osMutexId_t gEepromMutexHandle;
extern osMutexId_t gLoggerMutexHandle;
extern osEventFlagsId_t gControllerEventGroupHandle;
extern osEventFlagsId_t INITIALIZATION_COMPLETEHandle;


// Queues
extern osMessageQueueId_t xWaterControllerQueueHandle;
extern osMessageQueueId_t xLightControllerQueueHandle;
extern osMessageQueueId_t xWebSocketQueueHandle;
extern osMessageQueueId_t xHardwareQueueHandle;
extern osMessageQueueId_t xWaterCommandQueueHandle;
extern osMessageQueueId_t xLightCommandQueueHandle;



typedef struct {
    uint8_t message_type;
    uint8_t device;
    uint8_t target;
    uint8_t action;
    uint16_t value;
} MessageForWebSocket;

typedef enum {
    PHASE_FULL,
    PHASE_EMPTY
} WateringPhase;

// Deine anderen Deklarationen und Makros...
typedef enum {
    WATER_COMMAND_SET_STATE,
    // Weitere Befehle können hier hinzugefügt werden
} WaterCommandType;

typedef enum {
    WATER_STATE_FULL,
    WATER_STATE_EMPTY,
    // Weitere Zustände können hier hinzugefügt werden
} WaterState;

typedef enum {
    LIGHT_COMMAND_SET_INTENSITY
} LightCommandType;

typedef struct {
    LightCommandType commandType;
    uint8_t intensity;
} LightCommand;

typedef struct {
    WaterCommandType commandType;
    WaterState desiredState;
} WaterCommand;

typedef enum {
    COMMAND_CONTROL_PUMP,
    COMMAND_CONTROL_LIGHT,
} HardwareCommandType;

typedef struct {
    HardwareCommandType commandType;
    uint8_t deviceId;
    bool enable;
    uint8_t intensity;
} HardwareCommand;

typedef enum {
    LIGHT_OFF,
    LIGHT_ON
} LightState;

#endif // GLOBALS_H
