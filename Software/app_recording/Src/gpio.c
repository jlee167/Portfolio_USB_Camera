#include "stm32f7xx_hal.h"

void init_gpio(void)
{
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);

  for (int cnt = 0; cnt < 200;)
  {
    HAL_Delay(10);
    if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3) == GPIO_PIN_RESET)
      cnt = 0;
    else
      cnt++;
  }

  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
  HAL_Delay(50);
}



void disable_gpio_irq_all(void)
{
  HAL_NVIC_DisableIRQ(EXTI2_IRQn);
  HAL_NVIC_DisableIRQ(EXTI3_IRQn);
}


void restart_gpio_irq_all(void)
{
	HAL_NVIC_EnableIRQ(EXTI2_IRQn);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
  HAL_NVIC_ClearPendingIRQ(EXTI2_IRQn);
  HAL_NVIC_ClearPendingIRQ(EXTI3_IRQn);
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
}