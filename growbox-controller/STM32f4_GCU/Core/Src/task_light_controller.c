#include "cmsis_os.h"
#include "controller_state.h"
#include "uart_redirect.h"
#include <stdio.h>
#include "main.h"
#include "task_light_controller.h"
#include "schedules.h"
#include "task_state_manager.h"


void PWM_SetDutyCycle(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t DutyCycle)
{
    // Invertiere den DutyCycle-Wert
    uint32_t InvertedDutyCycle = 100 - DutyCycle;

    // Berechne den Pulse-Wert basierend auf der Periodenwert
    uint32_t Pulse = (InvertedDutyCycle * (htim->Init.Period + 1)) / 100;

    printf("task_light_controller.c:\tset dutyciclye to: %lu\r\n", Pulse);

    __HAL_TIM_SET_COMPARE(htim, Channel, Pulse);
}

void UpdateLightControllerState(int lightIntensity) {
	printf("task_light_controller.c:\tUpdate LightIntensity to: %d\r\n", lightIntensity);

    osMutexAcquire(gControllerStateMutex, osWaitForever);

    if (gControllerState.lightIntensity != lightIntensity) {
        gControllerState.lightIntensity = lightIntensity;
        printf("task_light_controller.c: changed lightIntensity in gControllerState to %d\r\n", lightIntensity);

        osEventFlagsSet(gControllerEventGroup, LIGHT_INTESITY_CHANGED_BIT);
    }
    osMutexRelease(gControllerStateMutex);
}

void ControlLight(int lightIntensity) {
	printf("task_light_controller.c: send new PWM to controller\r\n");
	PWM_SetDutyCycle(&LED_DIM_TIM, LED_DIM_CHANNEL, lightIntensity);

}

void StartLightTask(void *argument)
{
    printf("task_light_control.c: Starting Light Control Task\r\n");

    uint8_t lightIntensity = 0;

    // Initialisiere den PWM-Timer
    if (HAL_TIM_PWM_Start(&LED_DIM_TIM, LED_DIM_CHANNEL) != HAL_OK) {
        // Fehlerbehandlung, falls der Timer nicht gestartet werden kann
        printf("task_light_controller.c: Error! Unable to start LED Timer\r\n");
        Error_Handler();
    }

    // Licht ausschalten als Standard
    PWM_SetDutyCycle(&LED_DIM_TIM, LED_DIM_CHANNEL, 0);

    // Variablen für die Lichtsteuerung
    GrowCycleConfig growConfig;
    bool configLoaded = false;
    bool finished = false;

    // Lade die Konfiguration
    if (load_grow_cycle_config(&growConfig)) {
        printf("task_light_control.c: Successfully loaded GrowCycle from EEPROM\r\n");
        configLoaded = true;
    } else {
        printf("task_light_control.c: Failed to load grow cycle config\r\n");
        // Du kannst hier entscheiden, ob du weitermachen oder warten möchtest
    }

    uint8_t scheduleIndex = 0;
    uint8_t repetitionIndex = 0;
    bool lightOn = false;
    uint32_t delayTime = 0;

    for(;;)
    {
        // Warte auf Nachrichten in der LightController-Queue, mit Timeout
        if (osMessageQueueGet(xLightControllerQueueHandle, &lightIntensity, NULL, 100) == osOK) {
            // Nachricht drucken
            printf("task_light_controller.c:\tNeuer Wert lightIntensity: %d\r\n", lightIntensity);

            // Update Controller State
            UpdateLightControllerState(lightIntensity);

            // Hier kannst du entscheiden, ob du die aktuelle Lichtsteuerung unterbrechen möchtest
            // Zum Beispiel, wenn du manuell die Lichtintensität einstellen möchtest
        }

        printf("task_light_controller.c: os config loaded? %d\r\n", configLoaded);

        if (configLoaded) {
            // Verarbeite die Lichtzeitpläne
            if (scheduleIndex < growConfig.ledScheduleCount) {
                LedSchedule schedule = growConfig.ledSchedules[scheduleIndex];

                if (repetitionIndex < schedule.repetition) {
                    if (!lightOn) {
                        // Licht einschalten
                        printf("task_light_control.c: Turning light ON (Schedule %d, Repetition %d)\r\n", scheduleIndex, repetitionIndex);
                        UpdateLightControllerState(100);
                        ControlLight(100);
                        lightOn = true;
                        delayTime = schedule.durationOn * 1000; // in Millisekunden
                    } else {
                        // Licht ausschalten
                        printf("task_light_control.c: Turning light OFF (Schedule %d, Repetition %d)\r\n", scheduleIndex, repetitionIndex);
                        UpdateLightControllerState(0);
                        ControlLight(0);
                        lightOn = false;
                        delayTime = schedule.durationOff * 1000; // in Millisekunden

                        repetitionIndex++;
                    }
                } else {
                    // Nächster Zeitplan
                    scheduleIndex++;
                    repetitionIndex = 0;
                }
            } else {
                // Alle Zeitpläne abgeschlossen
                printf("task_light_control.c: All LED schedules completed\r\n");
                // Hier kannst du entscheiden, ob du die Zeitpläne erneut durchlaufen möchtest
                // scheduleIndex = 0;
                // repetitionIndex = 0;
                configLoaded = false; // Oder warte auf neue Konfiguration
                finished = true;
            }
        } else {
            // Versuche, die Konfiguration erneut zu laden
            if (load_grow_cycle_config(&growConfig) && !finished) {
                printf("task_light_control.c: Successfully loaded GrowCycle from EEPROM\r\n");
                configLoaded = true;
                scheduleIndex = 0;
                repetitionIndex = 0;
                lightOn = false;
            } else {
                printf("task_light_control.c: Failed to load grow cycle config\r\n");
                osDelay(5000); // Warte 5 Sekunden, bevor du es erneut versuchst
            }
        }

        // Wartezeit entsprechend einstellen
        printf("task_light_control.c: current delay time: %ld\r\n", delayTime);
        if (delayTime > 0) {
            //osDelay(delayTime);
            vTaskDelay(delayTime / portTICK_PERIOD_MS);
            delayTime = 0;
        } else {
            // Wenn keine Verzögerung erforderlich ist, eine kleine Pause einlegen, um nicht die CPU zu belasten
            //osDelay(100);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}
