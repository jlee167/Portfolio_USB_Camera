#include "dsp.h"



arm_biquad_casd_df1_inst_f32 biquad_config;
float32_t iir_states[4];

float32_t in_z1, in_z2, out_z1, out_z2;



iir_biquad_coeff coeff_lowpass_20khz = {
	.a0 = 0.8118496248713318f,
	.a1 = 1.6236992497426637f,
	.a2 = 0.8118496248713318f,
	.b1 = 1.588572206276975f,
	.b2 = 0.6588262932083526f
};


void __conv_to_cmsis_coeff(iir_biquad_coeff coeff_in, float32_t coeff_out[5]) {
	coeff_out[0] = coeff_in.a0;
	coeff_out[1] = coeff_in.a1;
	coeff_out[2] = coeff_in.a2;
	coeff_out[3] = (-coeff_in.b1);
	coeff_out[4] = (-coeff_in.b2);
}



int16_t calc_iir (int16_t inSample, iir_biquad_coeff coeff) {
	float inSampleF = (float)inSample;
	float outSampleF =
			  coeff.a0 * inSampleF
			+ coeff.a1 * in_z1
			+ coeff.a2 * in_z2
			- coeff.b1 * out_z1
			- coeff.b2 * out_z2;
	in_z2 = in_z1;
	in_z1 = inSampleF;
	out_z2 = out_z1;
	out_z1 = outSampleF;

	return (int16_t) outSampleF;
}


void init_lpf(void) {
	float32_t cmsis_biquad_coeffs_lpf_20k[5];
	__conv_to_cmsis_coeff(coeff_lowpass_20khz, cmsis_biquad_coeffs_lpf_20k);
	
	arm_biquad_cascade_df1_init_f32(&biquad_config, 1, cmsis_biquad_coeffs_lpf_20k, iir_states);
}


void filter_lpf_iir(float32_t *buf_in, float32_t *buf_out, int len) {
	arm_biquad_cascade_df1_f32(&biquad_config, buf_in, buf_out, len);
}