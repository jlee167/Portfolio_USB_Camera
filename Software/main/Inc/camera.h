#ifndef __CAMERA_H__
#define __CAMERA_H__


#include "stm32f7xx_hal.h"

void init_camera(void);
void start_camera(void);
void stop_camera(void);
void dcmi_to_fs(DCMI_HandleTypeDef *hdcmi);
void app_background_camera(DCMI_HandleTypeDef *hdcmi);


#endif