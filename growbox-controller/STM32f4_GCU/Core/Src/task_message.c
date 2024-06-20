#include "task_message.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "uart_redirect.h"
#include <stdlib.h>


#include <stdlib.h>

void StartMessageTask(void *argument) {
    char *message;
    for (;;) {
        // Warte auf Nachrichten in der Message-Queue
        if (osMessageQueueGet(xMessageQueueHandle, &message, NULL, osWaitForever) == osOK) {
            // Nachricht drucken
            printf("Received message: %s\n", message);

            // Free the allocated memory for the message
            free(message);
        }
    }
}
