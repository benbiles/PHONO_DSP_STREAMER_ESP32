// DSP runner Ben Biles

#ifndef _DSPRUNNER_H_
#define _DSPRUNNER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif


// BEN proto for DSP setup propergate IIR DSP biquad values

// run this in main to change freqncy and Q of LPF

esp_err_t DSP_setup(float freq, float q);



// BEN proto fixed biquads DSP setup
void DSP_setup_fixedBiquad(float b0, float b1, float b2, float a1, float a2);



// BEN creates Audio Element handle

audio_element_handle_t Dsp_init();



// this Configuration is just here for referance.. can go


/**
 * @brief      Equalizer Configuration
 */
typedef struct equalizer_cfg {
	int samplerate; /*!< Audio sample rate (in Hz)*/
	int channel; /*!< Number of audio channels (Mono=1, Dual=2) */
	int *set_gain; /*!< Equalizer gain */
	int out_rb_size; /*!< Size of output ring buffer */
	int task_stack; /*!< Task stack size */
	int task_core; /*!< Task running in core...*/
	int task_prio; /*!< Task priority*/
} equalizer_cfg_t;

#define EQUALIZER_TASK_STACK       (4 * 1024)
#define EQUALIZER_TASK_CORE        (0)
#define EQUALIZER_TASK_PRIO        (5)
#define EQUALIZER_RINGBUFFER_SIZE  (8 * 1024)


#define DEFAULT_EQUALIZER_CONFIG() {                \
        .samplerate  = 48000,                       \
        .channel     = 2,                           \
        .set_gain    = set_value_gain,              \
        .out_rb_size = EQUALIZER_RINGBUFFER_SIZE,   \
        .task_stack  = EQUALIZER_TASK_STACK,        \
        .task_core   = EQUALIZER_TASK_CORE,         \
        .task_prio   = EQUALIZER_TASK_PRIO,         \
    }


#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE





#endif
