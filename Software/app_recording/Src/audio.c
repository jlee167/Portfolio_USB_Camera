#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "fatfs.h"
#include "configs.h"
#include "led.h"
#include "state_macros.h"

extern I2S_HandleTypeDef hi2s1;

volatile int i2s_int_cnt = 0;
volatile int i2s_fs_cnt = 0;
volatile bool i2s_file_opened = false;

uint8_t  i2s_queue_head = 0;
uint8_t  i2s_queue_tail = 0;
uint16_t audio_sample_buf[I2S_QUEUE_LEN][I2S_BUFSIZE_HALFWORD];
uint16_t audio_fwrite_buf[I2S_SAMPLE_PER_INT];

volatile FRESULT result_audio_write 	= FR_DENIED;
volatile FRESULT result_audio_close		= FR_DENIED;
volatile FRESULT result_audio_open 		= FR_DENIED;


char audio_filename[30];


char* get_audio_filename(void) {
	return audio_filename;
}


uint8_t getNextI2sQueueTail() {
	const uint8_t is_last_idx = (i2s_queue_tail == (I2S_QUEUE_LEN-1));
	return is_last_idx ? 0 : i2s_queue_tail+1;
}


uint8_t getNextI2sQueueHead() {
	const uint8_t is_last_idx = (i2s_queue_head == (I2S_QUEUE_LEN-1));
	return is_last_idx ? 0 : i2s_queue_head+1;
}


void init_audio(void) {
	HAL_I2S_DeInit(&hi2s1);
}


void app_audio_callback(I2S_HandleTypeDef *hi2s) {
	i2s_int_cnt++;
	
	const bool i2s_queue_full = (i2s_queue_head == getNextI2sQueueTail());
	const uint8_t is_last_idx = (i2s_queue_tail == (I2S_QUEUE_LEN-1));
	
	if (i2s_queue_full) {
		HAL_I2S_Receive_DMA(hi2s, audio_sample_buf[i2s_queue_tail], I2S_BUFSIZE_WORD);
		return;
	}
	
	i2s_queue_tail = getNextI2sQueueTail();
	
	/* Issue I2S Read */
	while (HAL_I2S_GetState(hi2s) != HAL_I2S_STATE_READY) {};
	HAL_I2S_Receive_DMA(hi2s, audio_sample_buf[i2s_queue_tail], I2S_BUFSIZE_WORD);
}




void i2s_to_fs() {
	volatile int start_idx = 0;
	volatile int len = 0;
	UINT bw;
	volatile uint16_t first_val;
	volatile uint16_t val;
	
	if (i2s_queue_head == i2s_queue_tail)
		return;
	
	/* Search for first half-word data of i2s audio buffer */
	for (start_idx = 0; start_idx < 4; start_idx++) {
		if ((audio_sample_buf[i2s_queue_head][start_idx] & 0x1fff) != 0x0000)
			break;
	}
	first_val = audio_sample_buf[i2s_queue_head][start_idx];
	
	/* Current I2S setting: 18 bits sample from 32 bit data frame. 
	   Extract most significant 16 bit only. */
	for (len = 0; (len*4 + start_idx) < I2S_SAMPLE_PER_INT; len++) { 
		int idx_buf = len;
		int idx_sample = len*4 + start_idx;
		uint16_t wval =   ((audio_sample_buf[i2s_queue_head][idx_sample] << 1) 
											| (audio_sample_buf[i2s_queue_head][idx_sample+1] >> 15));
		audio_fwrite_buf[len] = wval;
		val = audio_sample_buf[i2s_queue_head][(len*4+start_idx)];
	}	
	
	result_audio_write = f_write(&SDFile, (void *) audio_fwrite_buf, len, &bw);	
	if (result_audio_write != FR_OK) {
		f_close(&SDFile);
		result_audio_open = f_open(&SDFile, audio_filename, FA_WRITE | FA_OPEN_APPEND);
	}
	
	if (!(i2s_fs_cnt%100)) {
		f_close(&SDFile);
		result_audio_open = f_open(&SDFile, audio_filename, FA_WRITE | FA_OPEN_APPEND);
	}
	
	i2s_queue_head = getNextI2sQueueHead();
	
	i2s_fs_cnt++;
}




void start_audio(void) {
	
	/* Open FatFS filestreams */
	char img_idx_str[20]="";
	sprintf(img_idx_str, "%d", 2);  
	strcpy(audio_filename, "");
	strcat(audio_filename, "/audio");
	strcat(audio_filename, img_idx_str);
	strcat(audio_filename, ".pcm");		
	result_audio_open = f_open(&SDFile, audio_filename, FA_WRITE | FA_OPEN_APPEND);
	
	if (result_audio_open != FR_OK) {
		led_err();
		return;
	}
	i2s_file_opened = true;
	
	
	/* Start I2S audio */
	if (HAL_I2S_Init(&hi2s1) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_NVIC_EnableIRQ(SPI1_IRQn);
	HAL_I2S_Receive_DMA(&hi2s1, audio_sample_buf[i2s_queue_tail], I2S_BUFSIZE_WORD);
}



void stop_audio(void) {
	if (HAL_I2S_DeInit(&hi2s1) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	HAL_NVIC_ClearPendingIRQ(SPI1_IRQn);
	HAL_Delay(500);
}


void app_background_audio(AppState state){
	if (state != RUNNING) {
		if (i2s_file_opened) {
			i2s_file_opened = false;
			result_audio_close = FR_DENIED;
			while (result_audio_close != FR_OK) {
				result_audio_close = f_close(&SDFile);
			}
		}

		i2s_queue_head = 0;
		i2s_queue_tail = 0;
	}

	if (i2s_queue_head != i2s_queue_tail) {
		i2s_to_fs();
	}
}