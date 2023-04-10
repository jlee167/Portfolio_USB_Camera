#include "stm32f7xx_hal.h"


/********************************
		LED Control Functions
*********************************/

void led_err() {
	for (int i = 0; i < 10; i++){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
		HAL_Delay(100);
	}
}

void led_fault() {
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
}


void led_success() {
	for (int i = 0; i < 2; i++){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_Delay(500);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
		HAL_Delay(500);
	}
}

void led_ready() {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
}


void led_stop() {
	for (int i = 0; i < 4; i++){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_Delay(200);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
		HAL_Delay(200);
	}
}


void toggle_flashlight(void) {
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_6);
}