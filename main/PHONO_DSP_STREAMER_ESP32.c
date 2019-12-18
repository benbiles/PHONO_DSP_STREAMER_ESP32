/* PHONO_DSP_STREAMER_ESP32
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

#include "equalizer.h"  // we may as well try the esp-adf equalizer to !

#include "esp_dsp.h"
#include "esp_system.h"





static const char *TAG = "PHONO_DSP_STREAMER_ESP32";





 #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE  // change if needed, default is CONFIG_LOG_DEFAULT_LEVEL




void app_main(void)
{




	// setup handles

	audio_pipeline_handle_t pipeline;


	// Ben add in "equalizer" element here if we use equalizer

    audio_element_handle_t i2s_stream_reader,DspProcessor,i2s_stream_writer; // equalizer disabled


    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);


    // init codec ic



    ESP_LOGI(TAG, "[ 1 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();

    // AUDIO_HAL_ADC_INPUT_LINE1  LINE1 is ADC channel 1 ( mic input 1 and 2 )



    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    audio_hal_set_volume(board_handle->audio_hal,100);

    ESP_LOGI(TAG, "[ 2 ] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();



    pipeline = audio_pipeline_init(&pipeline_cfg);



    ESP_LOGI(TAG, "[3.2] Create i2s stream to read data from codec chip");
    i2s_stream_cfg_t i2s_cfg_read = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg_read.type = AUDIO_STREAM_READER;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg_read);


   //   This is done in equalizer.c

   //   DspProcessor = audio_element_init(&DspCfg);


    ESP_LOGI(TAG, "[3.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);



  //     equalizer_cfg_t eq_cfg = DEFAULT_EQUALIZER_CONFIG();


// this sets the multi-band equaliser gains ( simple test )
// 31 Hz, 62 Hz, 125 Hz, 250 Hz, 500 Hz, 1 kHz, 2 kHz, 4 kHz, 8 kHz 16 kHz

  // set to approx RIIA phono curve
  // added 7db to whole range as example notes said min gain -13db
  // compensate ? pull input gain to this element -7db?

    //   int set_gain[] = { 27, 23, 19, 15, 12, 7, 6, 3, -3, -13,   // left
    //   		           27, 23, 19, 15, 12, 7, 6, 3, -3, -13};  // right

// The size of gain array should be the multiplication of NUMBER_BAND and number channels of audio stream data.
// The minimum of gain is -13 dB ??

  //   eq_cfg.set_gain = set_gain;

  //   equalizer = equalizer_init(&eq_cfg);



    //BEN initialize the DSP IIR filter code hacked into equalizer.c
    // make proper element DSPprocess.c and DSPprocess.h if this works
    DspProcessor = Dsp_init();


    ESP_LOGI(TAG, "[3.3] Register all elements to audio pipeline");

    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s_read");


       // BEN audio pipeline equalizer  ( using DSP IIR instead )
 //    audio_pipeline_register(pipeline, equalizer, "equalizer");


	audio_pipeline_register(pipeline, DspProcessor, "DspProcessor");


    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s_write");



    ESP_LOGI(TAG, "[3.4] Link it together [codec_chip]-->i2s_stream_reader-->DspProcessor-->i2s_stream_writer-->[codec_chip]");

    audio_pipeline_link(pipeline, (const char *[]) {"i2s_read", "DspProcessor", "i2s_write"}, 3); // N components in audio pipeline




//BEN tries to relink DspProcessor HOPEFULLY WE WON'T NEED THIS NOW ?

//	const char * boomer[] = {"i2s_read","DspProcessor","i2s_write" };
//  audio_pipeline_relink(pipeline, boomer, 3);



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
