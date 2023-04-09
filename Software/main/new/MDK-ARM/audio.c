#include "fatfs.h"
#include "file_gen.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>



/*volatile FRESULT result_write;
volatile FRESULT result_close;
volatile FRESULT result_open;

char audio_filename[30]; 
uint16_t audio_buf[2048];

extern I2S_HandleTypeDef hi2s1;


void init_audio(void) {
	int rec_idx = get_rec_idx();
	char img_idx_str[20]="";
	
	
	sprintf(img_idx_str, "%d", rec_idx);  
	
	strcpy(audio_filename, "/");
	strcpy(audio_filename, img_idx_str);
	strcat(audio_filename, "/audio");
	strcat(audio_filename, img_idx_str);
	strcat(audio_filename, ".pcm");		
	
	result_open = f_open(&SDFile, audio_filename, FA_WRITE | FA_CREATE_ALWAYS);
}

void process_audio() {
	while(HAL_I2S_GetState(&hi2s1) != HAL_I2S_STATE_READY) {};
	HAL_I2S_Receive_DMA(&hi2s1, audio_buf, 512);
	int audio_head = 0;
	for (audio_head = 0; audio_head < 512; audio_head++) {
		if (audio_buf[audio_head] != 0x0000)
			break;
	}
}

void stop_audio() {
	
}*/
