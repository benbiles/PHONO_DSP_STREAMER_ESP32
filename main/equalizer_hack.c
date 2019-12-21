// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <math.h>
#include <assert.h>
#include <stdint.h>



#include <string.h>
#include "esp_log.h"
#include "audio_error.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "esp_equalizer.h"
#include "equalizer.h"
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
    int  samplerate;
    int  channel;
    int  *set_gain;
    int  num_band;
    int  use_xmms_original_freqs;
    unsigned char *buf;
    void *eq_handle;
    int  byte_num;
    int  at_eof;
    int  gain_flag;
} equalizer_t;

int set_value_gain[NUMBER_BAND * 2]={-13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13};

#ifdef DEBUG_EQUALIZER_ENC_ISSUE
static FILE *infile;
#endif


/// BEN start DSP HACK


// should match bit rate in pipeline?


static float coeffs_lpf[5];
float w_lpf[5]; // dsp delay line , set accoriding to IIR filter ?

#define NNN 2048

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
float *pw_lpf      = w_lpf;

float graphFreq;
float graphQ;

audio_element_info_t Dsp_info = { 0 };


static esp_err_t Dsp_open(audio_element_handle_t self)
	{
	audio_element_getinfo(self, &Dsp_info);
	return ESP_OK;
	}


static esp_err_t Dsp_close(audio_element_handle_t self)
	{
	return ESP_OK;
	}

audio_element_err_t Dsp_read(audio_element_handle_t el, char *buf, int len, unsigned int wait_time, void *ctx)
	{
	return (audio_element_err_t)len;
	}


audio_element_err_t Dsp_write(audio_element_handle_t el, char *buf, int len, unsigned int wait_time, void *ctx)
	{
	return (audio_element_err_t)len;
	}




esp_err_t DSP_setup(float freq, float qFactor)
{

	// Calculate iir filter coefficients ( instead of preset )

	// generate low pass filter

	esp_err_t ret = ESP_OK;

	//DSP delay line
	w_lpf[0] = 2;
	w_lpf[1] = 2;

	ret = dsps_biquad_gen_hpf_f32(pcoeffs_lpf, freq, qFactor);

	if (ret  != ESP_OK) {
	ESP_LOGE(TAG, "Operation error dsps_biquad_gen_lpf_f32  = %i", ret);
	return ret;
	}


	ESP_LOGI(TAG, "Generated Biquad IIR Coefficiants success" );

	ESP_LOGI(TAG," B0 %f \r\n" , coeffs_lpf[0]);
	ESP_LOGI(TAG," B1 %f \r\n" , coeffs_lpf[1]);
	ESP_LOGI(TAG," B2 %f \r\n" , coeffs_lpf[2]);
	ESP_LOGI(TAG," A0 %f \r\n" , coeffs_lpf[3]);
	ESP_LOGI(TAG," A1 %f \r\n" , coeffs_lpf[4]);

	ESP_LOGI(TAG," dsp delay line 0 %f \r\n" , w_lpf[0]);
	ESP_LOGI(TAG," dsp delay line 1 %f \r\n" , w_lpf[1]);

	// numbers for graph
	graphFreq = freq;
	graphQ    = qFactor;

	return ret;
}



void DSP_setup_fixedBiquad(float b0,float b1,float b2,float a1,float a2)
{
	coeffs_lpf[0] = b0;
	coeffs_lpf[1] = b1;
	coeffs_lpf[2] = b2;
	coeffs_lpf[3] = a1;
	coeffs_lpf[4] = b2;
	coeffs_lpf[5] = 1; // a0 should = 1

	//DSP delay line
	w_lpf[0] = 2;
	w_lpf[1] = 2;

	ESP_LOGI(TAG, "Pre Defined Biquad IIR Coefficiants LOAD success \r\n" );

		ESP_LOGI(TAG," b0 %f \r\n" , coeffs_lpf[0]);
		ESP_LOGI(TAG," b1 %f \r\n" , coeffs_lpf[1]);
		ESP_LOGI(TAG," b2 %f \r\n" , coeffs_lpf[2]);
		ESP_LOGI(TAG," a1 %f \r\n" , coeffs_lpf[3]);
		ESP_LOGI(TAG," a2 %f \r\n" , coeffs_lpf[4]);
		ESP_LOGI(TAG," a0 %f \r\n" , coeffs_lpf[5]);

		ESP_LOGI(TAG," dsp delay line 0 %f \r\n" , w_lpf[0]);
		ESP_LOGI(TAG," dsp delay line 1 %f \r\n" , w_lpf[1]);

	return;
}



// ********** PROCESS the buffer with DSP IIR !!

static audio_element_err_t Dsp_process(audio_element_handle_t self, char *inbuf, int len)
	{

// Audio samples input
 audio_element_input(self, (char *)DspBuf, len);



// *************    DSP Process DspBuf here ********************************


// ****** len is meshured in bytes ( buffer leghth is half for int16_t )
bufSize = len/2;


 // check array and clean it
 if ( checkArray == 0 ) {
 // Generate d function as input signal
 dsps_d_gen_f32(FloatDspBuf, bufSize, 0);
 checkArray = 1;
 }


// convert 16bit audio smaples to float ****************
for ( int i = 0; i < bufSize; i++ )
{
FloatDspBuf[i] = ((float)DspBuf[i]) / (float)32768;
// FloatDspBuf[i] = FloatDspBuf[i] * devideit; // half vale ?
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
rett = dsps_biquad_f32(pFloatDspBuf,pFloatDspBufB,bufSize,pcoeffs_lpf,pw_lpf);

if (rett  != ESP_OK) { ESP_LOGE(TAG, "DSP IIR Operation error = %i", rett);
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
 for ( int z = 0; z < bufSize; z++ )
 {
 FloatDspBufB[z] = FloatDspBufB[z] * devideitOUT;  // half, depends on delay line size !!

 if ( FloatDspBufB[z] >1 )
 {
	 ESP_LOGI(TAG,"WARN out of range > 1 ");
	 FloatDspBufB[z] = 1;
 }
 if ( FloatDspBufB[z] < -1 )
 {
	 ESP_LOGI(TAG,"WARN out of range < -1 ");
	 FloatDspBufB[z] = -1;
 }}




// Show IIR filter results once
if ( checkGraph == 0 ) {

  ESP_LOGI(TAG, "Impulse response of IIR filter with F=%f, qFactor=%f", graphFreq, graphQ);
  dsps_view(pFloatDspBufB, 128, 64, 10,  -1, 1, 'x');

  checkGraph = 1;
}


/*
// bypass dsp  ****************
 for ( int z = 0; z < bufSize; z++ )
 {
 FloatDspBufB[z] = FloatDspBuf[z];
 }
*/


// convert float audio samples back into 16bit audio samples for pipeline
for ( int j = 0; j < bufSize; j++ )
{
FloatDspBufB[j] = FloatDspBufB[j] * 32767 ;
if( FloatDspBufB[j] > 32767 ) FloatDspBufB[j] = 32768;
if( FloatDspBufB[j] < -32768 ) FloatDspBufB[j] = -32768;
DspBuf[j] = (int16_t)FloatDspBufB[j]; // cast back
}


/// ************* END DSP Process ********************************



// Audio samples output
	int ret = audio_element_output(self, (char *)DspBuf, len);
	return (audio_element_err_t)ret;
	}



static esp_err_t Dsp_destroy(audio_element_handle_t self)
	{
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





/// EQUALIZER CODE BELOW HERE


static esp_err_t is_valid_equalizer_samplerate(int samplerate)
{
    if ((samplerate != 11025)
        && (samplerate != 22050)
        && (samplerate != 44100)
        && (samplerate != 48000)) {
        ESP_LOGE(TAG, "The sample rate should be only 11025Hz, 22050Hz, 44100Hz, 48000Hz, here is %dHz. (line %d)", samplerate, __LINE__);
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static esp_err_t is_valid_equalizer_channel(int channel)
{
    if ((channel != 1)
        && (channel != 2)) {
        ESP_LOGE(TAG, "The number of channels should be only 1 or 2, here is %d. (line %d)", channel, __LINE__);
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t equalizer_set_info(audio_element_handle_t self, int rate, int ch)
{
    equalizer_t *equalizer = (equalizer_t *)audio_element_getdata(self);
    if (equalizer->samplerate == rate && equalizer->channel == ch) {
        return ESP_OK;
    }
    if ((is_valid_equalizer_samplerate(rate) != ESP_OK)
        || (is_valid_equalizer_channel(ch) != ESP_OK)) {
        return ESP_ERR_INVALID_ARG;
    } else {
        ESP_LOGI(TAG, "The reset sample rate and channel of audio stream are %d %d.", rate, ch);
        equalizer->gain_flag = 1; 
        equalizer->samplerate = rate;
        equalizer->channel = ch;
    }
    return ESP_OK;
}

esp_err_t equalizer_set_gain_info(audio_element_handle_t self, int index, int value_gain, bool is_channels_gain_equal)
{   
    equalizer_t *equalizer = (equalizer_t *)audio_element_getdata(self);
    if ((equalizer->channel == 2)
        && (is_channels_gain_equal == true)) {
        if ((index < 0)
            || (index > NUMBER_BAND)){
            ESP_LOGE(TAG, "The range of index for audio gain of equalizer should be [0 9]. Here is %d. (line %d)", index, __LINE__);
            return ESP_ERR_INVALID_ARG;
        }
        int pos_index_channel = index + (equalizer->channel - 1) * NUMBER_BAND;
        if ((equalizer->set_gain[index] == value_gain)
            && (equalizer->set_gain[pos_index_channel] == value_gain)) {
            return ESP_OK;
        }
        ESP_LOGI(TAG, "The reset gain[%d] and gain[%d] of audio stream are %d.", index, pos_index_channel, value_gain);
        equalizer->gain_flag = 1;
        equalizer->set_gain[index] = value_gain;
        equalizer->set_gain[pos_index_channel] = value_gain;
        return ESP_OK;
    } else {
        if ((index < 0)
            || (index > NUMBER_BAND *2)){
            ESP_LOGE(TAG, "The number of channels is %d. The range of index for audio gain of equalizer should be [0 %d]. Here is %d. (line %d)", equalizer->channel,(equalizer->channel * NUMBER_BAND - 1), index, __LINE__);
            return ESP_ERR_INVALID_ARG;
        }
        if (equalizer->set_gain[index] == value_gain) {
            return ESP_OK;
        }
        ESP_LOGI(TAG, "The reset gain[%d] of audio stream is %d.", index, value_gain);
        equalizer->gain_flag = 1;
        equalizer->set_gain[index] = value_gain;
        return ESP_OK;
    }
}




static esp_err_t equalizer_destroy(audio_element_handle_t self)
{
    equalizer_t *equalizer = (equalizer_t *)audio_element_getdata(self);
    audio_free(equalizer);
    return ESP_OK;
}


static esp_err_t equalizer_open(audio_element_handle_t self)
{
#ifdef EQUALIZER_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
    ESP_LOGD(TAG, "equalizer_open");
    equalizer_t *equalizer = (equalizer_t *)audio_element_getdata(self);
    if (equalizer->set_gain == NULL) {
        ESP_LOGE(TAG, "The gain array should be set. (line %d)", __LINE__);
        return ESP_ERR_INVALID_ARG; 
    }
    audio_element_info_t info = {0};
    audio_element_getinfo(self, &info);
    if (info.sample_rates
        && info.channels) {
        equalizer->samplerate = info.sample_rates;
        equalizer->channel = info.channels;
    }
    equalizer->num_band = NUMBER_BAND;
    equalizer->use_xmms_original_freqs = USE_XMMS_ORIGINAL_FREQENT;
    equalizer->eq_handle = NULL;
    equalizer->at_eof = 0;
    equalizer->gain_flag = 0;
    if (equalizer->num_band != 10
        && equalizer->num_band != 15
        && equalizer->num_band != 25
        && equalizer->num_band != 31) {
        ESP_LOGE(TAG, "The number of bands should be one of 10, 15, 25, 31, here is %d. (line %d)", equalizer->num_band, __LINE__);
        return ESP_ERR_INVALID_ARG;
    }
    if (is_valid_equalizer_samplerate(equalizer->samplerate) != ESP_OK
        || is_valid_equalizer_channel(equalizer->channel) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    if (equalizer->use_xmms_original_freqs != 0
        && equalizer->use_xmms_original_freqs != 1) {
        ESP_LOGE(TAG, "The use_xmms_original_freqs should be only 0 or 1, here is %d. (line %d)", equalizer->use_xmms_original_freqs, __LINE__);
        return ESP_ERR_INVALID_ARG;
    }
    equalizer->buf = (unsigned char *)calloc(1, BUF_SIZE);
    if (equalizer->buf == NULL) {
        ESP_LOGE(TAG, "calloc buffer failed. (line %d)", __LINE__);
        return ESP_ERR_NO_MEM;
    }
    memset(equalizer->buf, 0, BUF_SIZE);
    equalizer->eq_handle = esp_equalizer_init(equalizer->channel, equalizer->samplerate, equalizer->num_band,
                                              equalizer->use_xmms_original_freqs);
    if (equalizer->eq_handle == NULL) {
        ESP_LOGE(TAG, "failed to do equalizer initialization. (line %d)", __LINE__);
        return ESP_FAIL;
    }
    for (int i = 0; i < equalizer->channel; i++) {
        for (int j = 0; j < NUMBER_BAND; j++) {
            esp_equalizer_set_band_value(equalizer->eq_handle, equalizer->set_gain[NUMBER_BAND * i + j], j, i);
        }
    }

#ifdef DEBUG_EQUALIZER_ENC_ISSUE
    char fileName[100] = {'//', 's', 'd', 'c', 'a', 'r', 'd', '//', 't', 'e', 's', 't', '.', 'p', 'c', 'm', '\0'};
    infile = fopen(fileName, "rb");
    if (!infile) {
        perror(fileName);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}





static esp_err_t equalizer_close(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "equalizer_close");
    equalizer_t *equalizer = (equalizer_t *)audio_element_getdata(self);
    esp_equalizer_uninit(equalizer->eq_handle);
    if(equalizer->buf == NULL){
        audio_free(equalizer->buf);
        equalizer->buf = NULL;
    }   

#ifdef EQUALIZER_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
#ifdef DEBUG_EQUALIZER_ENC_ISSUE
    fclose(infile);
#endif

    return ESP_OK;
}




static int equalizer_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    equalizer_t *equalizer = (equalizer_t *)audio_element_getdata(self);
    int ret = 0;
    if (equalizer->gain_flag == 1){
        equalizer_close(self);
        if (ret != ESP_OK) {
            return AEL_PROCESS_FAIL;
        }
        equalizer_open(self);
        if (ret != ESP_OK) {
            return AEL_PROCESS_FAIL;
        }
        ESP_LOGI(TAG, "Reopen equalizer");
        return ESP_CODEC_ERR_CONTINUE;
    }
    int r_size = 0;    
    if (equalizer->at_eof == 0) {
#ifdef DEBUG_EQUALIZER_ENC_ISSUE
        r_size = fread((char *)equalizer->buf, 1, BUF_SIZE, infile);
#else
        r_size = audio_element_input(self, (char *)equalizer->buf, BUF_SIZE);
#endif
    }
    if (r_size > 0) {
        if (r_size != BUF_SIZE) {
            equalizer->at_eof = 1;
        }
        equalizer->byte_num += r_size;
        ret = esp_equalizer_process(equalizer->eq_handle, (unsigned char *)equalizer->buf, r_size,
                                    equalizer->samplerate, equalizer->channel);
        ret = audio_element_output(self, (char *)equalizer->buf, BUF_SIZE);
    } else {
        ret = r_size;
    }
    return ret;
}



audio_element_handle_t equalizer_init(equalizer_cfg_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "config is NULL. (line %d)", __LINE__);
        return NULL;
    }
    equalizer_t *equalizer = audio_calloc(1, sizeof(equalizer_t));
    AUDIO_MEM_CHECK(TAG, equalizer, return NULL);     
    if (equalizer == NULL) {
        ESP_LOGE(TAG, "audio_calloc failed for equalizer. (line %d)", __LINE__);
        return NULL;
    }


    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.destroy = equalizer_destroy;
    cfg.process = equalizer_process;
    cfg.open = equalizer_open;
    cfg.close = equalizer_close;
    cfg.buffer_len = 0;
    cfg.tag = "equalizer";
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_rb_size = config->out_rb_size;
    audio_element_handle_t el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, {audio_free(equalizer); return NULL;});
    equalizer->samplerate = config->samplerate;
    equalizer->channel = config->channel;
    equalizer->set_gain = config->set_gain;
    audio_element_setdata(el, equalizer);
    audio_element_info_t info = {0};
    audio_element_setinfo(el, &info);
    ESP_LOGD(TAG, "equalizer_init");
    return el;
   }





