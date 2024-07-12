#include "jsmn_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "controller_state.h"
#include <stdlib.h>
#include "jsmn.h"

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
    printf("task_network.c:\t Raw data: %s\r\n", data);

    // Initialisiere den JSON-Parser
    jsmn_init(&parser);
    token_count = jsmn_parse(&parser, data, strlen(data), tokens, JSON_TOKENS);

    // Überprüfe das Ergebnis des Parsens
    if (token_count < 0) {
        printf("task_network.c:\t Failed to parse JSON: %d\r\n", token_count);
        if (token_count == JSMN_ERROR_NOMEM) {
            printf("task_network.c:\t Not enough tokens were provided.\r\n");
        } else if (token_count == JSMN_ERROR_INVAL) {
            printf("task_network.c:\t Invalid character inside JSON string.\r\n");
        } else if (token_count == JSMN_ERROR_PART) {
            printf("task_network.c:\t The string is not a full JSON packet, more bytes expected.\r\n");
        }
        return;
    }

    // Stelle sicher, dass das erste Token ein Objekt ist
    if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
        printf("task_network.c:\t Object expected\r\n");
        return;
    }

    // Debug: Ausgabe aller Token
    for (int i = 0; i < token_count; i++) {
        printf("task_network.c:\t Token %d: Type: %d, Start: %d, End: %d, Size: %d\r\n",
               i, tokens[i].type, tokens[i].start, tokens[i].end, tokens[i].size);
    }

    // Variablen zum Speichern der Daten aus dem JSON
    char message_type[20] = {0};
    char target[30] = {0};
    char action[20] = {0};
    bool value = false;
    int int_value = 0; // Neue Variable für Integer-Wert

    // Schleife über alle Tokens im JSON-Dokument
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(data, &tokens[i], "message_type") == 0) {
            // Extrahiere message_type
            snprintf(message_type, sizeof(message_type), "%.*s", tokens[i + 1].end - tokens[i + 1].start, data + tokens[i + 1].start);
            printf("task_network.c:\t Parsed message_type: %s\r\n", message_type);
            i++;
        } else if (jsoneq(data, &tokens[i], "payload") == 0) {
            // Extrahiere die Payload
            printf("task_network.c:\t Extracting payload...\r\n");
            for (int j = i + 1; j < token_count; j++) {
                if (tokens[j].type == JSMN_OBJECT && tokens[j].size == 0) {
                    printf("task_network.c:\t End of payload object\r\n");
                    break; // End of payload object
                }

                if (jsoneq(data, &tokens[j], "target") == 0) {
                    snprintf(target, sizeof(target), "%.*s", tokens[j + 1].end - tokens[j + 1].start, data + tokens[j + 1].start);
                    printf("task_network.c:\t Parsed target: %s\r\n", target);
                    j++;
                } else if (jsoneq(data, &tokens[j], "action") == 0) {
                    snprintf(action, sizeof(action), "%.*s", tokens[j + 1].end - tokens[j + 1].start, data + tokens[j + 1].start);
                    printf("task_network.c:\t Parsed action: %s\r\n", action);
                    j++;
                } else if (jsoneq(data, &tokens[j], "value") == 0) {
                    if (strncmp(data + tokens[j + 1].start, "true", 4) == 0) {
                        value = true;
                    } else if (strncmp(data + tokens[j + 1].start, "false", 5) == 0) {
                        value = false;
                    } else {
                        // Versuche, den Wert als Integer zu parsen
                        int_value = atoi(data + tokens[j + 1].start);
                    }
                    //printf("task_network.c:	Parsed value: %s\r\n", value ? "true" : "false");
                    printf("task_network.c:\t Parsed int_value: %d\r\n", int_value);
                    j++;
                }
            }
            printf("task_network.c:\t End of payload object processing\r\n");
            i = token_count; // Beende die Schleife nach dem Verarbeiten der Payload
        }
    }

    // Verarbeite die extrahierten Daten basierend auf dem message_type und den anderen Feldern
    if (strcmp(message_type, "control_command") == 0) {
        if (strcmp(action, "change") == 0) {
            printf("task_network.c:\t Executing change command for target: %s\r\n", target);

            osMutexAcquire(gControllerStateMutex, osWaitForever);

            if (strcmp(target, "wasserbeckenZustand") == 0) {
                bool current_value = gControllerState.wasserbeckenZustand;
                printf("task_network.c:\t Current wasserbeckenZustand: %s\r\n", current_value ? "true" : "false");

                if (current_value != value) {
                    printf("task_network.c:\t Changing wasserbeckenZustand from %s to %s\r\n", current_value ? "true" : "false", value ? "true" : "false");
                    gControllerState.wasserbeckenZustand = value;
                    printf("task_network.c:\t Updated wasserbeckenZustand to %s\r\n", gControllerState.wasserbeckenZustand ? "true" : "false");
                    osEventFlagsSet(gControllerEventGroup, WATER_STATE_CHANGED_BIT);
                } else {
                    printf("task_network.c:\t No change needed for wasserbeckenZustand\r\n");
                }
            }

            if (strcmp(target, "lightIntensity") == 0) {
                int current_value = gControllerState.lightIntensity;
                printf("task_network.c:\t Current lightIntensity: %d\r\n", current_value);

                if (current_value != int_value) {
                    printf("task_network.c:\t Changing lightIntensity from %d to %d\r\n", current_value, int_value);
                    gControllerState.lightIntensity = int_value;
                    printf("task_network.c:\t Updated lightIntensity to %d\r\n", gControllerState.lightIntensity);
                    osEventFlagsSet(gControllerEventGroup, LIGHT_INTESITY_CHANGED_BIT);
                } else {
                    printf("task_network.c:	No change needed for lightIntensity\r\n");
                }
            }

            osMutexRelease(gControllerStateMutex);
        }
    } else if (strcmp(message_type, "newGrowCycle") == 0) {
    	printf("task_network.c:	Received new GrowCycle\r\n");

        if (strcmp(action, "add_growCycle") == 0) {

            if (strcmp(target, "growCycle") == 0) {
                printf("task_network.c:\t Target is %s", target);
            }
        }
    }
}
