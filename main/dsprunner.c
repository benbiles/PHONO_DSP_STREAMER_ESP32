// dsprunner.c  Ben Biles
/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
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

//  BEN adds dsp lib
#include "esp_dsp.h"

static const char *TAG = "EQUALIZER";

#define BUF_SIZE (100)
#define NUMBER_BAND (10)
#define USE_XMMS_ORIGINAL_FREQENT (0)

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


static float coeffs_lpf[5];
float w_lpf[5]; // dsp delay line , set accoriding to IIR filter ?

#define NNN 1024  // buffer size

static int checkArray = 0;
static int checkGraph = 0;

int bufSize; // len is in bytes , were using inst16 samples

int16_t DspBuf[NNN];
int16_t DspBufOut[NNN];

float FloatDspBufL[NNN/2];
float FloatDspBufR[NNN/2];

float FloatDspBufBL[NNN/2];
float FloatDspBufBR[NNN/2];

// pointers for dsp lib
const float *pFloatDspBufL = FloatDspBufL;
const float *pFloatDspBufR = FloatDspBufR;

const float *pFloatDspBufBL = FloatDspBufBL;
const float *pFloatDspBufBR = FloatDspBufBR;


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

	ret = dsps_biquad_gen_lpf_f32(pcoeffs_lpf, freq, qFactor);

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

static audio_element_err_t Dsp_process(audio_element_handle_t self, char *inbuf,int len) {

// Audio samples input
audio_element_input(self, (char*) DspBuf, len);

// *************    DSP Process DspBuf here ********************************

// ****** len is meshured in bytes ( buffer leghth is half for int16_t )

	bufSize = len /2; // int16 bufsize


// convert 16bit audio smaples to float ****************
		int x=0;
	for (int i = 0; i < bufSize; i=i+2) {
		FloatDspBufL[x] = ((float) DspBuf[i]) / 32768;
	    FloatDspBufR[x] = ((float) DspBuf[i+1]) / 32768;
	    x++;
	}

 // DSP IIR biquad process
 esp_err_t rett = ESP_OK;

 //****** less crackle without /2  ? why ?
 int dspBufSize = bufSize;  // LEFT or RIGHT samples so half again


rett = dsps_biquad_f32_ae32(pFloatDspBufL,FloatDspBufBL,dspBufSize,pcoeffs_lpf,pw_lpf);

	if (rett != ESP_OK) {
		ESP_LOGE(TAG, "DSP IIR LEFT channel Operation error = %i", rett);
		return rett;
	}

	esp_err_t rettb = ESP_OK;
rettb = dsps_biquad_f32_ae32(pFloatDspBufR,FloatDspBufBR,dspBufSize,pcoeffs_lpf,pw_lpf);

	if (rettb != ESP_OK) {
		ESP_LOGE(TAG, "DSP IIR LEFT Operation error = %i", rettb);
		return rettb;
	}

/*
	 // ANCI C BIQUAD IIR more CPU cycles same thing !!
	 for (int i = 0 ; i < bufSize ; i++)
	 {
	 const float d0 = FloatDspBufL[i] - coeffs_lpf[3] * w_lpf[0] - coeffs_lpf[4] * w_lpf[1];
	 FloatDspBufBL[i] = coeffs_lpf[0] * d0 +  coeffs_lpf[1] * w_lpf[0] + coeffs_lpf[2] * w_lpf[1];
	 w_lpf[1] = w_lpf[0];
	 w_lpf[0] = d0;

	  const float e0 = FloatDspBufR[i] - coeffs_lpf[3] * w_lpf[0] - coeffs_lpf[4] * w_lpf[1];
	 FloatDspBufBR[i] = coeffs_lpf[0] * e0 +  coeffs_lpf[1] * w_lpf[0] + coeffs_lpf[2] * w_lpf[1];
	 w_lpf[1] = w_lpf[0];
	 w_lpf[0] = e0;
	 }
*/


// Show IIR filter results once
	if (checkGraph == 0) {

		ESP_LOGI(TAG, "IIR filter LEFT with F=%f, qFactor=%f",
				graphFreq, graphQ);
		dsps_view(pFloatDspBufBL, 128, 64, 10, -1, 1, 'x');

	    ESP_LOGI(TAG, "IIR filter RIGHT with F=%f, qFactor=%f",
				graphFreq, graphQ);
		dsps_view(pFloatDspBufBR, 128, 64, 10, -1, 1, 'x');

		checkGraph = 1;
		}

	int z =0;
	for (int pp = 0; pp < bufSize; pp=pp+2) {
	DspBufOut[pp] = FloatDspBufBL[z]* 32767; // cast back
	DspBufOut[pp+1] = FloatDspBufBR[z]* 32767; // cast back
	z++;
	}

	//**** WARN using DspBufOut[] to output now


/// ************* END DSP Process ********************************

// Audio samples output
	int ret = audio_element_output(self, (char*) DspBufOut, len);
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
	DspCfg.task_stack = (2 * 2046);
	DspCfg.task_prio = (5);
	DspCfg.task_core = (0); // core 1 has FPU issues lol
	DspCfg.out_rb_size = (8 * 1024);

	audio_element_handle_t DspProcessor = audio_element_init(&DspCfg);
	return DspProcessor;
}

// END BEN DSP functions
