/*
 * task_light_controller.h
 *
 *  Created on: Jun 26, 2024
 *      Author: chris
 */

#ifndef INC_TASK_LIGHT_CONTROLLER_H_
#define INC_TASK_LIGHT_CONTROLLER_H_

extern TIM_HandleTypeDef htim10;  // Deklaration von htim10

#define LED_DIM_TIM htim10  //
#define LED_DIM_CHANNEL TIM_CHANNEL_1


#endif /* INC_TASK_LIGHT_CONTROLLER_H_ */
