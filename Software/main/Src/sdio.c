#include "led.h"
#include "fatfs.h"
#include "audio.h"

volatile int fs_fail_cnt = 0;


extern SD_HandleTypeDef hsd1;


void sd_err_handler() {
	fs_fail_cnt++;
	led_err();
	
	FRESULT audio_closed = f_close(&SDFile);
	FRESULT video_closed = f_close(&VideoFile);
	
	HAL_SD_InitCard(&hsd1);
	f_mount(&SDFatFS, SDPath, 1);
	
	FRESULT audio_opened = f_open(&SDFile, get_audio_filename(), FA_WRITE | FA_OPEN_APPEND);
	if (audio_opened != FR_OK)
		sd_err_handler();
}


void init_sd_fatfs(void) {
	volatile FRESULT result_mount = FR_INVALID_PARAMETER;
	int mount_fail_cnt = 0;
	
	
	volatile HAL_StatusTypeDef result = HAL_SD_Init(&hsd1);
	while (result != HAL_OK) {
		result = HAL_SD_Init(&hsd1);
	}
	volatile HAL_StatusTypeDef result2 = HAL_SD_InitCard(&hsd1);
	result_mount = f_mount(&SDFatFS, SDPath, 1);
	
	
	while (result_mount != FR_OK) {
			HAL_SD_DeInit(&hsd1);
			HAL_SD_Init(&hsd1);
			HAL_SD_InitCard(&hsd1);
		
			result_mount = f_mount(&SDFatFS, SDPath, 1);
	};
	
	f_mkdir("image");
	f_mkdir("audio");
	HAL_Delay(50);
}