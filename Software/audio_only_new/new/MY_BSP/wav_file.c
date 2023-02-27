#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "fatfs.h"

#pragma pack(1)
#include "wav_file.h"


void init_default(WAVHeader *wav){
	strcpy(wav->riff_chunk_id, "RIFF");
	wav->chunk_size = 36;
	strcpy(wav->riff_format, "WAVE");
	strcpy(wav->fmt_chunk_id, "fmt ");
	wav->fmt_chunk_size = 16;
	wav->fmt_audio = FORMAT_PCM;
	wav->num_channels = 1;
	wav->sample_rate = 44100;
	wav->byte_rate = 88200; 
	wav->block_align = 2;
	wav->bit_rate = 16;
	strcpy(wav->subchunk2_id, "data");
	wav->subchunk2_size = 0;
}



FRESULT create_wav_fatfs(FIL *file, char *fname, WAVHeader *header){
	volatile FRESULT result;
	UINT bw;
	
	result = f_open(file, fname, FA_WRITE | FA_CREATE_ALWAYS | FA_READ);
	result = f_write(file, (void*)header, sizeof(WAVHeader), &bw);
	
	return result;
}


FRESULT __update_size_header(FIL *file, uint32_t *new_size){
	volatile FRESULT result;
	UINT bw;
	
	f_lseek(file, 4);
	result = f_write(file, (void*)new_size, sizeof(uint32_t), &bw);
	
	f_lseek(file, f_size(file));
	
	return result;
}


FRESULT write_pcm_data(FIL *file, int16_t *data, uint32_t len){
	volatile FRESULT result;
	UINT bw;
	UINT br;
	
	volatile uint32_t chunk_size;
	volatile uint32_t subchunk2_size;
	
	static int pcm_write_cnt = 0;
	static int pcm_err_cnt = 0;

	result = f_lseek(file, OFFSET_CHUNKSIZE);
	result = f_read(file, (char*) &chunk_size, sizeof(uint32_t), &br);
	result = f_lseek(file, OFFSET_SUBCHUNK2SIZE);
	result = f_read(file, (char*) &subchunk2_size, sizeof(uint32_t), &br);
	
	uint32_t new_subchunk2_size = subchunk2_size + len*2;
	uint32_t new_chunk_size = new_subchunk2_size + 36;

	
	f_lseek(file, f_size(file));
	result = f_write(file, (void*)data, len*2, &bw);
	
	if (result != FR_OK) {
		pcm_err_cnt++; 
	}
	
	f_lseek(file, OFFSET_CHUNKSIZE);
	result = f_write(file, (void*)&new_chunk_size, sizeof(uint32_t), &bw);
	f_sync(file);
	
	if (result != FR_OK) {
		pcm_err_cnt++; 
	}
	
	f_lseek(file, OFFSET_SUBCHUNK2SIZE);
	result = f_write(file, (void*)&new_subchunk2_size, sizeof(uint32_t), &bw);
	f_lseek(file, f_size(file));
	f_sync(file);
	
	if (result != FR_OK) {
		pcm_err_cnt++; 
	}
	
	pcm_write_cnt++;
	
	return result;
}

