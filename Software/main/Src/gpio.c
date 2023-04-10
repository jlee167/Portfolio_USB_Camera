#include "stm32f7xx_hal.h"


void init_gpio(void) {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	
	for (int cnt=0; cnt < 200; ) {  
		HAL_Delay(10);
		if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3) == GPIO_PIN_RESET){
			cnt = 0;
		} else {
			cnt++;
		}
	}
		
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
	HAL_Delay(50);
}