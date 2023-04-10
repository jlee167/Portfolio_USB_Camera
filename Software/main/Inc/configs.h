#define I2S_SAMPLE_RATE_HZ 		(44100)
#define I2S_SAMPLE_PER_INT 		(I2S_SAMPLE_RATE_HZ / 16)
#define I2S_BUFSIZE_SAMPLES 	(I2S_SAMPLE_PER_INT * 2)		// sampled in stereo, but only single channel used
#define I2S_BUFSIZE_HALFWORD	(I2S_BUFSIZE_SAMPLES * 2)		
#define I2S_BUFSIZE_WORD			(I2S_BUFSIZE_HALFWORD /2)	
#define I2S_QUEUE_LEN  				(3)



enum AppStatus{
	OK,
	SD_ERR
};