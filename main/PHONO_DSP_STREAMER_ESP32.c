/* PHONO_DSP_STREAMER_ESP32 ben biles 2019
 *
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// used for DSP
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_pipeline.h"
#include "i2s_stream.h"
#include "board.h"
#include "audio_hal.h"

#include "dsprunner.h"

#include "esp_dsp.h"
#include "esp_system.h"

#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

#include "es8388.h" // codec control !


static const char *TAG = "PHONO_DSP_STREAMER_ESP32";

 #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE  // change if needed, default is CONFIG_LOG_DEFAULT_LEVEL

float freq;
float qFactor;


void app_main(void)
{

	// 8byte double RIAA phono curve for 48khz
	double bb0 =  0.2275882473429072;
	double bb1 = -0.1680644758323426;
	double bb2 = -0.0408560856583673;
	double aa1 =  0.85803894915999625;
	double aa2 = -0.7179700042784745;

	// cast to 4byte
		float b0 = (float)bb0;
		float b1 = (float)bb1;
		float b2 = (float)bb2;
		float a1 = (float)aa1;
        float a2 = (float)aa2;

   // UNCOMMENT to load RIIA phono curve ( comment / disable DSP_setup below  )
   // if using must comment out DSP_setup(freq,qFactor);

   // DSP_setup_fixedBiquad(b0,b1,b2,a1,a2);


   // currently HPF, not LPF: set DSP EQ filter values here
  	freq =  0.3; // 0 -> 0.5
  	qFactor = 1; // 0 -> ?

  	// ** run this any time you want to change DSP filter

	 DSP_setup(freq,qFactor);


// SETUP HANDLES

	audio_pipeline_handle_t pipeline;


	// Ben add in "equalizer" element here if we use equalizer

    audio_element_handle_t i2s_stream_reader,DspProcessor,i2s_stream_writer; // equalizer disabled


    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);


    ESP_LOGI(TAG, "[ 1 ] Start codec chip");
     audio_board_handle_t board_handle = audio_board_init();
    // hal controls codec ic
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);


    // set ADC input gain  0 = 0db
        es8388_set_mic_gain(0);

        es8388_set_bits_per_sample(ES_MODULE_ADC_DAC,BIT_LENGTH_16BITS);

    ESP_LOGI(TAG, "[ 2 ] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGI(TAG, "[3.2] Create i2s stream to read data from codec chip");
    i2s_stream_cfg_t i2s_cfg_read = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg_read.type = AUDIO_STREAM_READER;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg_read);

    ESP_LOGI(TAG, "[3.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    // define DspProcessor
     DspProcessor = Dsp_init();


    ESP_LOGI(TAG, "[3.3] Register all elements to audio pipeline");

    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s_read");

	audio_pipeline_register(pipeline, DspProcessor, "DspProcessor");

    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s_write");


    ESP_LOGI(TAG, "[3.4] Link it together [codec_chip]-->i2s_stream_reader-->DspProcessor-->i2s_stream_writer-->[codec_chip]");

    audio_pipeline_link(pipeline, (const char *[]) {"i2s_read", "DspProcessor", "i2s_write"}, 3); // N components in audio pipeline


    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);


    // Print errors / events to console

    ESP_LOGI(TAG, "[ 6 ] Listen for all pipeline events");

    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
     //       ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.cmd == AEL_MSG_CMD_ERROR) {
       //     ESP_LOGE(TAG, "[ * ] Action command error: src_type:%d, source:%p cmd:%d, data:%p, data_len:%d", msg.source_type, msg.source, msg.cmd, msg.data, msg.data_len);
        	}

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
           // ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }


    //  ESP_LOGI(TAG, "[ 7 ] Stop audio_pipeline");
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, i2s_stream_reader);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_reader);
    audio_element_deinit(i2s_stream_writer);
}
