#include "jsmn_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "controller_state.h"
#include <stdlib.h>
#include "jsmn.h"
#include "led_cycles.h"
#include "at24cxx.h"


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

    // Ausgabe der empfangenen Rohdaten
    printf("jsmn_utils.c:\t Raw data: %s\r\n", data);

    // Initialisiere den JSON-Parser
    jsmn_init(&parser);
    token_count = jsmn_parse(&parser, data, strlen(data), tokens, JSON_TOKENS);

    // Überprüfe das Ergebnis des Parsens
    if (token_count < 0) {
        printf("jsmn_utils.c:\t Failed to parse JSON: %d\r\n", token_count);
        if (token_count == JSMN_ERROR_NOMEM) {
            printf("jsmn_utils.c:\t Not enough tokens were provided.\r\n");
        } else if (token_count == JSMN_ERROR_INVAL) {
            printf("jsmn_utils.c:\t Invalid character inside JSON string.\r\n");
        } else if (token_count == JSMN_ERROR_PART) {
            printf("jsmn_utils.c:\t The string is not a full JSON packet, more bytes expected.\r\n");
        }
        return;
    }
    printf("jsmn_utils.c:\t Successfully parsed JSON: %d tokens\r\n", token_count);

    // Stelle sicher, dass das erste Token ein Objekt ist
    if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
        printf("jsmn_utils.c:\t Object expected\r\n");
        return;
    }

    // Debug: Ausgabe aller Token
    for (int i = 0; i < token_count; i++) {
        printf("jsmn_utils.c:\t Token %d: Type: %d, Start: %d, End: %d, Size: %d\r\n",
               i, tokens[i].type, tokens[i].start, tokens[i].end, tokens[i].size);
    }

    // Variablen zum Speichern der Daten aus dem JSON
    char message_type[20] = {0};
    char target[30] = {0};
    char action[20] = {0};
    LEDCycleData ledData = {0}; // Variable für LED-Zyklen

    // Schleife über alle Tokens im JSON-Dokument
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(data, &tokens[i], "message_type") == 0) {
            // Extrahiere message_type
            snprintf(message_type, sizeof(message_type), "%.*s", tokens[i + 1].end - tokens[i + 1].start, data + tokens[i + 1].start);
            printf("jsmn_utils.c:\t Parsed message_type: %s\r\n", message_type);
            i++;
        } else if (jsoneq(data, &tokens[i], "payload") == 0) {
            // Extrahiere die Payload
            printf("jsmn_utils.c:\t Extracting payload...\r\n");
            for (int j = i + 1; j < token_count; j++) {
                if (tokens[j].type == JSMN_OBJECT && tokens[j].size == 0) {
                    printf("jsmn_utils.c:\t End of payload object\r\n");
                    break; // End of payload object
                }

                if (jsoneq(data, &tokens[j], "target") == 0) {
                    snprintf(target, sizeof(target), "%.*s", tokens[j + 1].end - tokens[j + 1].start, data + tokens[j + 1].start);
                    printf("jsmn_utils.c:\t Parsed target: %s\r\n", target);
                    j++;
                } else if (jsoneq(data, &tokens[j], "action") == 0) {
                    snprintf(action, sizeof(action), "%.*s", tokens[j + 1].end - tokens[j + 1].start, data + tokens[j + 1].start);
                    printf("jsmn_utils.c:\t Parsed action: %s\r\n", action);
                    j++;
                } else if (jsoneq(data, &tokens[j], "value") == 0) {
                    printf("jsmn_utils.c:\t Extracting value...\r\n");
                    for (int k = j + 1; k < token_count; k++) {
                        if (tokens[k].type == JSMN_OBJECT && tokens[k].size == 0) {
                            printf("jsmn_utils.c:\t End of value object\r\n");
                            break; // End of value object
                        }

                        if (jsoneq(data, &tokens[k], "ledCycles") == 0) {
                            printf("jsmn_utils.c:\t Extracting ledCycles...\r\n");
                            if (tokens[k + 1].type == JSMN_ARRAY) {
                                int array_size = tokens[k + 1].size;
                                printf("jsmn_utils.c:\t Number of ledCycles: %d\r\n", array_size);
                                int array_index = k + 2;  // Das erste Element des Arrays
                                ledData.numCycles = array_size;

                                for (int l = 0; l < array_size; l++) {
                                    printf("jsmn_utils.c:\t Processing cycle %d\r\n", l);
                                    if (tokens[array_index].type == JSMN_OBJECT) {
                                        for (int m = 1; m < tokens[array_index].size * 2; m += 2) {
                                            printf("jsmn_utils.c:\t  Key: %.*s\r\n", tokens[array_index + m].end - tokens[array_index + m].start, data + tokens[array_index + m].start);
                                            printf("jsmn_utils.c:\t  Value: %.*s\r\n", tokens[array_index + m + 1].end - tokens[array_index + m + 1].start, data + tokens[array_index + m + 1].start);
                                            if (jsoneq(data, &tokens[array_index + m], "durationOn") == 0) {
                                                ledData.cycles[l].durationOn = atoi(data + tokens[array_index + m + 1].start);
                                                printf("jsmn_utils.c:\t  Parsed durationOn: %d\r\n", ledData.cycles[l].durationOn);
                                            } else if (jsoneq(data, &tokens[array_index + m], "durationOff") == 0) {
                                                ledData.cycles[l].durationOff = atoi(data + tokens[array_index + m + 1].start);
                                                printf("jsmn_utils.c:\t  Parsed durationOff: %d\r\n", ledData.cycles[l].durationOff);
                                            } else if (jsoneq(data, &tokens[array_index + m], "ledRepetitions") == 0) {
                                                ledData.cycles[l].ledRepetitions = atoi(data + tokens[array_index + m + 1].start);
                                                printf("jsmn_utils.c:\t  Parsed ledRepetitions: %d\r\n", ledData.cycles[l].ledRepetitions);
                                            }
                                        }
                                        array_index += tokens[array_index].size * 2 + 1;  // Gehe zum nächsten Objekt im Array
                                    }
                                }
                            }
                        }
                    }
                    printf("jsmn_utils.c:\t End of value object processing\r\n");

                    // Zusätzliche Debug-Ausgabe der ledData-Inhalte
                    printf("jsmn_utils.c:\t LED Cycles extracted from value object:\r\n");
                    printf("jsmn_utils.c:\t Number of Cycles: %u\r\n", (unsigned int)ledData.numCycles);
                    for (size_t i = 0; i < ledData.numCycles; i++) {
                        printf("jsmn_utils.c:\t Cycle %lu: durationOn = %u, durationOff = %u, ledRepetitions = %u\r\n",
                               (unsigned long)i, ledData.cycles[i].durationOn, ledData.cycles[i].durationOff, ledData.cycles[i].ledRepetitions);
                    }

                    j = token_count; // Beende die Schleife nach dem Verarbeiten der value
                }
            }
            printf("jsmn_utils.c:\t End of payload object processing\r\n");
            i = token_count; // Beende die Schleife nach dem Verarbeiten der Payload
        }
    }

    // Verarbeite die extrahierten Daten basierend auf dem message_type und den anderen Feldern
    if (strcmp(message_type, "control_command") == 0) {
        // Vorheriger Code zum Verarbeiten von control_command
    } else if (strcmp(message_type, "newGrowCycle") == 0) {
        printf("jsmn_utils.c:\tReceived new GrowCycle\r\n");

        if (strcmp(action, "add_growCycle") == 0) {
            printf("jsmn_utils.c:\t action is add_growcycle (check: action: %s)\r\n", action);
            storeLEDCyclesInEEPROM(&ledData);
        }
    }
}
