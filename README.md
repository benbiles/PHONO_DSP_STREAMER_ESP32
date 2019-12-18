PHONO_DSP_STREAMER_ESP32
Phono Preamp DSP RIAA curve streamer for SONOS and other http HLS capable WiFi audio speakers

Initially designed to run on ESP32-LyraT hardware using esp-idf libs ( Espressif IoT Development Framework ) and esp-adf libs.

https://www.espressif.com/en/products/hardware/esp32-lyrat

esp-adf libraries https://github.com/espressif/esp-adf

The project will be modified to support better ADC and DAC hardware if DSP RIAA curve in DSP IIR biquads is succesful.


PHASE 1 testing / notes

now hacked equalizer.c ( see equalizer_hack.c in main folder.  

rename equalizer_hack.c to equalizer.c and replace in ESP-ADF framework , don't try to compile equalizer_hack.c in main  )

pipeline including DspProcessor element now working

audio_pipeline_link(pipeline, (const char *[]) {"i2s_read", "DspProcessor", "i2s_write"}, 3);


using dsps_iir_main.c DSP concept with esp-dsp libs. https://github.com/espressif/esp-dsp

notes:

float coeffs_lpf[5]; // load known biquad coefficiants here rather than generate them in code ?

float w_lpf[5] = {0,0}; // we don't need delay for biquad filter ?

float coeffs_lpf[0] = 0.105263157894737;

float coeffs_lpf[1] = -0.076417573949578;

float coeffs_lpf[2] = -0.024632736829188;

fwoat coeffs_lpf[3] = 1.866859545059558;

float coeffs_lpf[4] = -0.867284262601157;


// process samples in array with DSP IIR biquad RIIA phono curve

passthrough working with callbacks in place for DspBuf
TO DO COMPLETE THIS:

in equalizer.c

samples are in what format when device is running in 32bit 96khz ?

float -1 to +1 ? or int32_t ?

float DspBuf[4096]

dsps_biquad_f32_ae32(DspBuf,DspBuf,len,DSP_iir_coeffs,DSP_delay);



PHASE 2.

No obvious http HLS streaming server example in esp-adf libs. Needs investigating.

would it make more sense to push stream to SONOS server?

notes:

Matrix-Voice-ESP32-MQTT-Audio-Streamer ( its a slightly different dev board but its ESP32 so same environment.

https://github.com/Romkabouter/Matrix-Voice-ESP32-MQTT-Audio-Streamer/blob/master/MatrixVoiceAudioServer/MatrixVoiceAudioServer.ino
