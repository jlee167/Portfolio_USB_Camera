#ifndef __DSP_H__
#define __DSP_H__

#include "arm_math.h"


typedef struct iir_biquad_coeff {
	float32_t a0;
	float32_t a1;
	float32_t a2;
	float32_t b1;
	float32_t b2;
} iir_biquad_coeff;


int16_t calc_iir (int16_t inSample, iir_biquad_coeff coeff);

void init_lpf(void);
void filter_lpf_iir(float32_t *buf_in, float32_t *buf_out, int len);



#endif