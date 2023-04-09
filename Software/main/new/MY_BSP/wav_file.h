#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "fatfs.h"

#ifndef	__WAV_FILE_H__
#define __WAV_FILE_H__

#define FORMAT_PCM						(0X0001)
#define OFFSET_CHUNKSIZE			(4)
#define OFFSET_SUBCHUNK2SIZE 	(40)

#define SUBCHUNK_ID  					("data")


typedef struct wav_file{
	char riff_chunk_id[4];
	uint32_t chunk_size;
	char riff_format[4];
	char fmt_chunk_id[4];
	uint32_t fmt_chunk_size;
	uint16_t fmt_audio;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bit_rate;
	char subchunk2_id[4];
	uint32_t subchunk2_size;
} WAVHeader;


typedef struct wav_data{
	char chunk_id[4];
	uint32_t chunk_size;
	uint8_t *data;
} WAVData;


void init_default(WAVHeader *wav);
void init_wav_data(WAVData *data);

FRESULT create_wav_fatfs(FIL *file, char *fname, WAVHeader *header);
FRESULT __update_size_header(FIL *file, uint32_t *new_size);
FRESULT write_pcm_data(FIL *file, int16_t *data, uint32_t len);

#endif