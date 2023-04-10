#include "stm32f7xx_hal.h"
#include <stdbool.h>

char* get_audio_filename(void);
void i2s_to_fs(void);
void start_audio(void);
void stop_audio(void);
void app_audio_callback(I2S_HandleTypeDef *hi2s);
void app_background_audio(bool running);
