#include "jsmn_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "controller_state.h"
#include <stdlib.h>
#include "jsmn.h"
#include "led_cycles.h"
#include "at24cxx.h"
#include "helper_websocket.h"
#include "globals.h"




extern osMutexId_t gEEPROMMutexHandle;


bool eraseStatus = false;
uint8_t  wData[] = "PENIS World 123";
uint8_t  rData[25];
#define MEM_ADDR    0x00u




void debugEEPROMData(const LEDCycleData* data) {
    printf("Debugging EEPROM Data:\r\n");
    printf("Number of Cycles: %u\r\n", (unsigned int)data->numCycles);
    for (size_t i = 0; i < data->numCycles; i++) {
        printf("Cycle %lu: durationOn = %u, durationOff = %u, ledRepetitions = %u\r\n",
               (unsigned long)i, data->cycles[i].durationOn, data->cycles[i].durationOff, data->cycles[i].ledRepetitions);
    }
}

void printBytes(const uint8_t* data, size_t size) {
    printf("Bytes: ");
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", data[i]);
    }
    printf("\r\n");
}

void storeLEDCyclesInEEPROM(const LEDCycleData* ledData) {
    printf("jsmn_utils.c:\tcheck if at24 i2c storage is connected...");

    // Versuchen, den Mutex zu erwerben und den Erfolg direkt überprüfen
    if (osMutexAcquire(gEEPROMMutexHandle, osWaitForever) == osOK) {
        if(at24_isConnected()) {
            printf("connected\r\n");

            // Größe der zu schreibenden Daten berechnen
            size_t dataSize = sizeof(*ledData);
            printf("jsmn_utils-c:\t Size of LEDCycleData: %lu bytes\r\n", (unsigned long)dataSize);

            // Debug-Ausgabe der Daten, die gespeichert werden
            printf("jsmn_utils.c:\t Saving LED Cycles to EEPROM\r\n");
            printf("jsmn_utils.c:\t Number of Cycles: %u\r\n", (unsigned int)ledData->numCycles);

            for (size_t i = 0; i < ledData->numCycles; i++) {
                printf("jsmn_utils.c:\t Cycle %lu: durationOn = %u, durationOff = %u, ledRepetitions = %u\r\n",
                (unsigned long)i, ledData->cycles[i].durationOn, ledData->cycles[i].durationOff, ledData->cycles[i].ledRepetitions);
            }

            // Schreiben der Daten in das EEPROM
            printf("jsmn_utils.c:\t Data size to write: %lu bytes\r\n", (unsigned long)dataSize);
            printf("jsmn_utils.c:\t LEDCycleData address: %p\r\n", (void*)ledData);

            int writeStatus = at24_write(MEM_ADDR, (uint8_t*)ledData, dataSize, 1000);
            vTaskDelay(10 / portTICK_PERIOD_MS);

            // Überprüfen, ob das Schreiben erfolgreich war
            if (writeStatus == 1) {
                printf("jsmn_utils.c:\t Successfully wrote %lu bytes to EEPROM\r\n", (unsigned long)dataSize);

                // Lesen der Daten aus dem EEPROM zum Verifizieren
                LEDCycleData readData = {0};
                vTaskDelay(10 / portTICK_PERIOD_MS);

                int readStatus = at24_read(MEM_ADDR, (uint8_t*)&readData, dataSize, 1000);

                if (readStatus == 1) {
                    printf("jsmn_utils.c:\t Successfully read %lu bytes from EEPROM\r\n", (unsigned long)dataSize);
                    printf("jsmn_utils.c:\t Number of Cycles (read back): %u\r\n", (unsigned int)readData.numCycles);

                    for (size_t i = 0; i < readData.numCycles; i++) {
                        printf("jsmn_utils.c:\t Cycle %lu (read back): durationOn = %u, durationOff = %u, ledRepetitions = %u\r\n",
                               (unsigned long)i, readData.cycles[i].durationOn, readData.cycles[i].durationOff, readData.cycles[i].ledRepetitions);
                    }

                    // Aktualisieren des Controller-Zustands
                    if (osMutexAcquire(gControllerStateMutex, osWaitForever) == osOK) {
                        gControllerState.readyForAutoRun = true;
                        osMutexRelease(gControllerStateMutex);
                        osEventFlagsSet(gControllerEventGroup, READY_FOR_AUTORUN_STATE_CHANGED_BIT);
                    }

                } else {
                    printf("jsmn_utils.c:\t Failed to read from EEPROM, error code: %d\r\n", readStatus);
                }
            } else {
                printf("jsmn_utils.c:\t Failed to write to EEPROM, error code: %d\r\n", writeStatus);
            }

        } else {
            printf("FAILED\r\n");
        }

        osMutexRelease(gEEPROMMutexHandle);
    } else {
        printf("jsmn_utils.c:\t Failed to get MUTEX!\r\n");
    }
}


// Hilfsfunktion zum Vergleichen von JSON-Schlüsseln
int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
    // Berechne die Länge des Tokens
    int len = tok->end - tok->start;

    // Überprüfe, ob das Token ein String ist
    if (tok->type == JSMN_STRING) {
        // Überprüfe, ob die Länge des Tokens mit der Länge des zu vergleichenden Strings übereinstimmt
        if ((int)strlen(s) == len) {
            // Vergleiche die Inhalte des Tokens und des zu vergleichenden Strings
            if (strncmp(json + tok->start, s, len) == 0) {
                return 0; // Die Strings sind gleich
            }
        }
    }

    return -1; // Die Strings sind nicht gleich
}




void process_received_data(const char* data) {
    jsmn_parser parser;
    jsmntok_t tokens[JSON_TOKENS];
    int token_count;
    int value = 0;

    // Ausgabe der empfangenen Rohdaten
    printf("jsmn_utils.c:\t Raw data: %s\r\n", data);

    // Initialisiere den JSON-Parser
    jsmn_init(&parser);
    token_count = jsmn_parse(&parser, data, strlen(data), tokens, JSON_TOKENS);

    // Überprüfe das Ergebnis des Parsens
    if (token_count < 0) {
        printf("jsmn_utils.c:\t Failed to parse JSON: %d\r\n", token_count);
        return;
    }
    printf("jsmn_utils.c:\t Successfully parsed JSON: %d tokens\r\n", token_count);

    // Stelle sicher, dass das erste Token ein Objekt ist
    if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
        printf("jsmn_utils.c:\t Object expected\r\n");
        return;
    }

    // Variablen zum Speichern der Daten aus dem JSON
    char uid[32] = {0};
    char message_type[20] = {0};
    char target[30] = {0};
    char action[20] = {0};

    // Schleife über alle Tokens im JSON-Dokument
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(data, &tokens[i], "UID") == 0) {
            // Extrahiere uid
            snprintf(uid, sizeof(uid), "%.*s", tokens[i + 1].end - tokens[i + 1].start, data + tokens[i + 1].start);
            printf("jsmn_utils.c:\t Found uid: %s\r\n", uid);
            i++;
        } else if (jsoneq(data, &tokens[i], "message_type") == 0) {
            // Extrahiere message_type
            snprintf(message_type, sizeof(message_type), "%.*s", tokens[i + 1].end - tokens[i + 1].start, data + tokens[i + 1].start);
            printf("jsmn_utils.c:\t Found message_type: %s\r\n", message_type);
            i++;
        } else if (jsoneq(data, &tokens[i], "target") == 0) {
            snprintf(target, sizeof(target), "%.*s", tokens[i + 1].end - tokens[i + 1].start, data + tokens[i + 1].start);
            printf("jsmn_utils.c:\t Found target: %s\r\n", target);
            i++;
        } else if (jsoneq(data, &tokens[i], "action") == 0) {
            snprintf(action, sizeof(action), "%.*s", tokens[i + 1].end - tokens[i + 1].start, data + tokens[i + 1].start);
            printf("jsmn_utils.c:\t Found action: %s\r\n", action);
            i++;
        } else if (jsoneq(data, &tokens[i], "value") == 0) {
            value = atoi(data + tokens[i + 1].start);
            printf("jsmn_utils.c:\t Found value: %d\r\n", value);
            i++;
        }
    }

    // Verarbeite die extrahierten Daten basierend auf dem message_type und den anderen Feldern
    if (strcmp(message_type, "update") == 0 && strcmp(target, "all") == 0 && strcmp(action, "init") == 0) {
        printf("jsmn_utils.c:\t Detected message_type: update, target: all, action: init\r\n");

        // Sende alle Werte des ControllerState nacheinander
        osMutexAcquire(gControllerStateMutex, osWaitForever);

        add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_WATER_LEVEL, ACTION_UPDATE, gControllerState.wasserbeckenZustand);
        add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_PUMPE_ZULAUF, ACTION_UPDATE, gControllerState.pumpeZulauf);
        add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_PUMPE_ABLAUF, ACTION_UPDATE, gControllerState.pumpeAblauf);
        add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_SENSOR_VOLL, ACTION_UPDATE, gControllerState.sensorVoll);
        add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_SENSOR_LEER, ACTION_UPDATE, gControllerState.sensorLeer);
        add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_LIGHT_INTENSITY, ACTION_UPDATE, gControllerState.lightIntensity);
        add_message_to_websocket_queue(MESSAGE_TYPE_UPDATE, DEVICE_CONTROLLER, TARGET_READYFORAUTORUN, ACTION_UPDATE, gControllerState.readyForAutoRun);


        osMutexRelease(gControllerStateMutex);
    } else if (strcmp(message_type, "control_command") == 0 && strcmp(action, "set") == 0) {
        printf("jsmn_utils.c:\t Detected message_type: control_command and action set for target: %s\r\n", target);

        osMutexAcquire(gControllerStateMutex, osWaitForever);

        if (strcmp(target, "wasserbeckenZustand") == 0) {
            gControllerState.wasserbeckenZustand = (value == 1);
            printf("jsmn_utils.c:\t Updated wasserbeckenZustand to %s\r\n", gControllerState.wasserbeckenZustand ? "true" : "false");
            osEventFlagsSet(gControllerEventGroup, WATER_STATE_CHANGED_BIT);
        } else if (strcmp(target, "LightIntensity") == 0) {
            gControllerState.lightIntensity = value;
            printf("jsmn_utils.c:\t Updated lightIntensity to %d\r\n", gControllerState.lightIntensity);
            osEventFlagsSet(gControllerEventGroup, LIGHT_INTESITY_CHANGED_BIT);
        }
        osMutexRelease(gControllerStateMutex);
    }
}



