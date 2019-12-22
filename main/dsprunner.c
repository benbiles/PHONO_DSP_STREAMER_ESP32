// dsprunner.c  Ben Biles

#include <math.h>
#include <assert.h>
#include <stdint.h>

#include <string.h>
#include "esp_log.h"
#include "audio_error.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "dsprunner.h"
#include "audio_type_def.h"

#include "esp_system.h"

//  BEN dsp lib
#include "esp_dsp.h"

static const char *TAG = "EQUALIZER";

#define BUF_SIZE (100)
#define NUMBER_BAND (10)
#define USE_XMMS_ORIGINAL_FREQENT (0)
// #define EQUALIZER_MEMORY_ANALYSIS
// #define DEBUG_EQUALIZER_ENC_ISSUE

typedef struct equalizer {
	int samplerate;
	int channel;
	int *set_gain;
	int num_band;
	int use_xmms_original_freqs;
	unsigned char *buf;
	void *eq_handle;
	int byte_num;
	int at_eof;
	int gain_flag;
} equalizer_t;

int set_value_gain[NUMBER_BAND * 2] = { -13, -13, -13, -13, -13, -13, -13, -13,
		-13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13 };

#ifdef DEBUG_EQUALIZER_ENC_ISSUE
static FILE *infile;
#endif

/// BEN start DSP HACK

// should match bit rate in pipeline?

static float coeffs_lpf[5];
float w_lpf[5]; // dsp delay line , set accoriding to IIR filter ?

#define NNN 4096

static int checkArray = 0;
static int checkGraph = 0;

int bufSize; // len is in bytes , change to int
int16_t DspBuf[NNN];
float FloatDspBuf[NNN];
float FloatDspBufB[NNN];

// pointers for dsp lib
const float *pFloatDspBuf = FloatDspBuf;
float *pFloatDspBufB = FloatDspBufB;
float *pcoeffs_lpf = coeffs_lpf;
float *pw_lpf = w_lpf;

float graphFreq;
float graphQ;

audio_element_info_t Dsp_info = { 0 };

static esp_err_t Dsp_open(audio_element_handle_t self) {
	audio_element_getinfo(self, &Dsp_info);
	return ESP_OK;
}

static esp_err_t Dsp_close(audio_element_handle_t self) {
	return ESP_OK;
}

audio_element_err_t Dsp_read(audio_element_handle_t el, char *buf, int len,
		unsigned int wait_time, void *ctx) {
	return (audio_element_err_t) len;
}

audio_element_err_t Dsp_write(audio_element_handle_t el, char *buf, int len,
		unsigned int wait_time, void *ctx) {
	return (audio_element_err_t) len;
}

esp_err_t DSP_setup(float freq, float qFactor) {

	// Calculate iir filter coefficients ( instead of preset )

	// generate low pass filter

	esp_err_t ret = ESP_OK;

	//DSP delay line
	w_lpf[0] = 2;
	w_lpf[1] = 2;

	ret = dsps_biquad_gen_hpf_f32(pcoeffs_lpf, freq, qFactor);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Operation error dsps_biquad_gen_lpf_f32  = %i", ret);
		return ret;
	}

	ESP_LOGI(TAG, "Generated Biquad IIR Coefficiants success");

	ESP_LOGI(TAG, " B0 %f \r\n", coeffs_lpf[0]);
	ESP_LOGI(TAG, " B1 %f \r\n", coeffs_lpf[1]);
	ESP_LOGI(TAG, " B2 %f \r\n", coeffs_lpf[2]);
	ESP_LOGI(TAG, " A0 %f \r\n", coeffs_lpf[3]);
	ESP_LOGI(TAG, " A1 %f \r\n", coeffs_lpf[4]);

	ESP_LOGI(TAG, " dsp delay line 0 %f \r\n", w_lpf[0]);
	ESP_LOGI(TAG, " dsp delay line 1 %f \r\n", w_lpf[1]);

	// numbers for graph
	graphFreq = freq;
	graphQ = qFactor;

	return ret;
}

void DSP_setup_fixedBiquad(float b0, float b1, float b2, float a1, float a2) {
	coeffs_lpf[0] = b0;
	coeffs_lpf[1] = b1;
	coeffs_lpf[2] = b2;
	coeffs_lpf[3] = a1;
	coeffs_lpf[4] = b2;
	coeffs_lpf[5] = 1; // a0 should = 1

	//DSP delay line
	w_lpf[0] = 2;
	w_lpf[1] = 2;

	ESP_LOGI(TAG, "Pre Defined Biquad IIR Coefficiants LOAD success \r\n");

	ESP_LOGI(TAG, " b0 %f \r\n", coeffs_lpf[0]);
	ESP_LOGI(TAG, " b1 %f \r\n", coeffs_lpf[1]);
	ESP_LOGI(TAG, " b2 %f \r\n", coeffs_lpf[2]);
	ESP_LOGI(TAG, " a1 %f \r\n", coeffs_lpf[3]);
	ESP_LOGI(TAG, " a2 %f \r\n", coeffs_lpf[4]);
	ESP_LOGI(TAG, " a0 %f \r\n", coeffs_lpf[5]);

	ESP_LOGI(TAG, " dsp delay line 0 %f \r\n", w_lpf[0]);
	ESP_LOGI(TAG, " dsp delay line 1 %f \r\n", w_lpf[1]);

	return;
}

// ********** PROCESS the buffer with DSP IIR !!

static audio_element_err_t Dsp_process(audio_element_handle_t self, char *inbuf,
		int len) {

// Audio samples input
	audio_element_input(self, (char*) DspBuf, len);

// *************    DSP Process DspBuf here ********************************

// ****** len is meshured in bytes ( buffer leghth is half for int16_t )
	bufSize = len / 2;

	// check array and clean it
	if (checkArray == 0) {
		// Generate d function as input signal
		dsps_d_gen_f32(FloatDspBuf, bufSize, 0);
		checkArray = 1;
	}

// convert 16bit audio smaples to float ****************
	for (int i = 0; i < bufSize; i++) {
		FloatDspBuf[i] = ((float) DspBuf[i]) / (float) 32768;
	}

	/*

	 float devideitIN = 0.5;
	 for ( int z = 0; z < bufSize; z++ )
	 {
	 FloatDspBuf[z] = FloatDspBuf[z] * devideitIN;  // half, depends on delay line size !!

	 if ( FloatDspBuf[z] >1 )
	 {
	 ESP_LOGI(TAG,"WARN out of range > 1 ");
	 FloatDspBuf[z] = 1;
	 }
	 if ( FloatDspBuf[z] < -1 )
	 {
	 ESP_LOGI(TAG,"WARN out of range < -1 ");
	 FloatDspBuf[z] = -1;
	 }}

	 */

// DSP IIR biquad process
	esp_err_t rett = ESP_OK;
	rett = dsps_biquad_f32(pFloatDspBuf, pFloatDspBufB, bufSize, pcoeffs_lpf,
			pw_lpf);

	if (rett != ESP_OK) {
		ESP_LOGE(TAG, "DSP IIR Operation error = %i", rett);
		return rett;
	}

	/*
	 // ANCI C BIQUAD IIR version working more CPU cycles !!

	 for (int i = 0 ; i < bufSize ; i++)
	 {
	 const float d0 = FloatDspBuf[i] - coeffs_lpf[3] * w_lpf[0] - coeffs_lpf[4] * w_lpf[1];
	 FloatDspBufB[i] = coeffs_lpf[0] * d0 +  coeffs_lpf[1] * w_lpf[0] + coeffs_lpf[2] * w_lpf[1];
	 w_lpf[1] = w_lpf[0];
	 w_lpf[0] = d0;
	 }
	 */

// half dsp outpt values sould be less than +/-1  ****************
	float devideitOUT = 0.5;
	for (int z = 0; z < bufSize; z++) {
		FloatDspBufB[z] = FloatDspBufB[z] * devideitOUT; // half, depends on delay line size !!

		if (FloatDspBufB[z] > 1) {
			ESP_LOGI(TAG, "DSP element WARNING out of range > 1 ");
			FloatDspBufB[z] = 1;
		}
		if (FloatDspBufB[z] < -1) {
			ESP_LOGI(TAG, "DSP element WARNING out of range < -1 ");
			FloatDspBufB[z] = -1;
		}
	}

// Show IIR filter results once
	if (checkGraph == 0) {

		ESP_LOGI(TAG, "Impulse response of IIR filter with F=%f, qFactor=%f",
				graphFreq, graphQ);
		dsps_view(pFloatDspBufB, 128, 64, 10, -1, 1, 'x');

		checkGraph = 1;
	}

	/*
	 // ****** WARNING ENABLE TO BYPASS DSP  ****************
	 for ( int z = 0; z < bufSize; z++ )
	 {
	 FloatDspBufB[z] = FloatDspBuf[z];
	 }
	 */

// convert float audio samples back into 16bit audio samples for pipeline
	for (int j = 0; j < bufSize; j++) {
		FloatDspBufB[j] = FloatDspBufB[j] * 32767;
		if (FloatDspBufB[j] > 32767)
			FloatDspBufB[j] = 32768;
		if (FloatDspBufB[j] < -32768)
			FloatDspBufB[j] = -32768;
		DspBuf[j] = (int16_t) FloatDspBufB[j]; // cast back
	}

/// ************* END DSP Process ********************************

// Audio samples output
	int ret = audio_element_output(self, (char*) DspBuf, len);
	return (audio_element_err_t) ret;
}

static esp_err_t Dsp_destroy(audio_element_handle_t self) {
	return ESP_OK;
}

audio_element_handle_t Dsp_init()  // no equalizer array to pass in !
{
	audio_element_cfg_t DspCfg; // = DEFAULT_AUDIO_ELEMENT_CONFIG();
	memset(&DspCfg, 0, sizeof(audio_element_cfg_t));
	DspCfg.destroy = Dsp_destroy;
	DspCfg.process = Dsp_process;
	DspCfg.read = Dsp_read; // why are these needed if they are not called?
	DspCfg.write = Dsp_write; // why are these needed if they are not called?
	DspCfg.open = Dsp_open;
	DspCfg.close = Dsp_close;
	DspCfg.buffer_len = (1024);
	DspCfg.tag = "Dsp";
	DspCfg.task_stack = (2 * 1024);
	DspCfg.task_prio = (5);
	DspCfg.task_core = (0); // core 1 has FPU issues lol
	DspCfg.out_rb_size = (8 * 1024);

	audio_element_handle_t DspProcessor = audio_element_init(&DspCfg);
	return DspProcessor;
}

// END BEN DSP functions

