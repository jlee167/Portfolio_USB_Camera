#include "fatfs.h"
#include "configs.h"

uint16_t audio_fwrite_buf[I2S_SAMPLE_PER_INT];

volatile FRESULT result_audio_write 	= FR_DENIED;
volatile FRESULT result_audio_close		= FR_DENIED;
volatile FRESULT result_audio_open 		= FR_DENIED;
volatile FRESULT result_video_write  	= FR_DENIED;
volatile FRESULT result_video_close  	= FR_DENIED;
volatile FRESULT result_video_open   	= FR_DENIED;