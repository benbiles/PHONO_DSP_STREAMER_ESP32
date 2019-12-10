/* Audio passthru

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
// used for DSP
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_dsp.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_pipeline.h"
#include "i2s_stream.h"
#include "board.h"
#include "audio_hal.h"

#include "equalizer.h"  // we may as well try the esp-adf equalizer to !




static const char *TAG = "PASSTHRU";

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE


// ben defines DSP iir phono curve

float coeffs_lpf[5]; // load known biquad coefficiants here rather than generate them in code ?
float w_lpf[5] = {0,0}; // we don't need delay for biquad filter ?
// ben defines phono curve IIR biquad
float coeffs_lpf[0] = 0.105263157894737;
float coeffs_lpf[1] = -0.076417573949578;
float coeffs_lpf[2] = -0.024632736829188;
float coeffs_lpf[3] = 1.866859545059558;
float coeffs_lpf[4] = -0.867284262601157;




void app_main(void)
{

	// setup handles

	audio_pipeline_handle_t pipeline;

    audio_element_handle_t i2s_stream_writer, i2s_stream_reader, equalizer;


    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);


    // init codec ic

    ESP_LOGI(TAG, "[ 1 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_LINE_IN, AUDIO_HAL_CTRL_START);


    ESP_LOGI(TAG, "[ 2 ] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);


    ESP_LOGI(TAG, "[3.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[3.2] Create i2s stream to read data from codec chip");
    i2s_stream_cfg_t i2s_cfg_read = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg_read.type = AUDIO_STREAM_READER;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg_read);




    equalizer_cfg_t eq_cfg = DEFAULT_EQUALIZER_CONFIG();

// this sets the multi-band equaliser gains ( simple test )
// 31 Hz, 62 Hz, 125 Hz, 250 Hz, 500 Hz, 1 kHz, 2 kHz, 4 kHz, 8 kHz 16 kHz
    int set_gain[] = { -13, -13, -13, -13, -13, 0, 0, 0, 0, 0,   // left less bass HP ?
    		           0, 0, 0, 0, 0, -13, -13, -13, -13, -13};  // right more bass LP ?

    eq_cfg.set_gain = set_gain; // The size of gain array should be the multiplication of NUMBER_BAND and number channels of audio stream data. The minimum of gain is -13 dB.
    equalizer = equalizer_init(&eq_cfg);




    ESP_LOGI(TAG, "[3.3] Register all elements to audio pipeline");

    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s_read");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s_write");
    // ben adds audio pipeline equalizer
    audio_pipeline_register(pipeline, equalizer, "equalizer");


 // BEN tries to process input audio signal using i2s_read pipe!!!
 //
 // not likely this will do work at all
 // we must understand how the i2s_stream_reader ( defined as i2s_read above ? ) is working

 // i2s_read is a function that reads whatver is currently in the DMA transfer buffer?

 // how about the timing ? usually we would read the buffer when its full ?

 // comment this block out for just testing the EQ

      unsigned int start_b = xthal_get_ccount();
      esp_err_t ret = dsps_biquad_f32(i2s_read, i2s_write, sizeOf(i2s_read), coeffs_lpf, w_lpf);
      unsigned int end_b = xthal_get_ccount();
      if (ret  != ESP_OK)
      {
          ESP_LOGE(TAG, "DSP iir Operation error = %i", ret);
          return;
      }

    // END BEN's DSP iir modify

    ESP_LOGI(TAG, "[3.4] Link it together [codec_chip]-->i2s_stream_reader-->equalizer-->i2s_stream_writer-->[codec_chip]");

    audio_pipeline_link(pipeline, (const char *[]) {"i2s_read", "equalizer", "i2s_write"}, 3); // N components in audio pipeline


    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

   // ESP_LOGI(TAG, "[ 6 ] Listen for all pipeline events");
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
