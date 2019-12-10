PHONO_DSP_STREAMER_ESP32
Phono Preamp DSP RIAA curve streamer for SONOS and other http HLS capable WiFi audio speakers

Initially designed to run on ESP32-LyraT hardware using esp-idf libs ( Espressif IoT Development Framework ) and esp-adf libs. https://www.espressif.com/en/products/hardware/esp32-lyrat

esp-adf libraries https://github.com/espressif/esp-adf

The project will be modified to support better ADC and DAC hardware if DSP RIAA curve in DSP IIR biquads is succesful.

PHASE 1 testing / notes

impliment passthrough example merged with dsps_iir_main.c DSP concept with esp-dsp libs. https://github.com/espressif/esp-dsp

passthrough example https://github.com/espressif/esp-adf/tree/master/examples/audio_processing/pipeline_passthru

DSP iir biquad example https://github.com/espressif/esp-dsp/blob/master/examples/iir/main/dsps_iir_main.c

starting point...

float coeffs_lpf[5]; // load known biquad coefficiants here rather than generate them in code ?

float w_lpf[5] = {0,0}; // we don't need delay for biquad filter ?

float coeffs_lpf[0] = 0.105263157894737;

float coeffs_lpf[1] = -0.076417573949578;

float coeffs_lpf[2] = -0.024632736829188;

fwoat coeffs_lpf[3] = 1.866859545059558;

float coeffs_lpf[4] = -0.867284262601157;

// process samples in array with DSP IIR biquad RIIA phono curve

dsps_biquad_f32(sampleArrayIN, sampleArrayOUT, Nsamples, *coeffs_lpf, *w_lpf);

pipe out sampleArrayOUT to DAC as in passthrough example

PHASE 2.

No obvoius http HLS streaming example in esp-adf libs. Needs investigating.

https://docs.espressif.com/projects/esp-adf/en/latest/api-reference/streams/#http-stream

take a look at this audio streamer code , it looks like it maybe chunking audio for HLS ?

Matrix-Voice-ESP32-MQTT-Audio-Streamer ( its a slightly differnt dev board but its ESP32 so same enviroment.

https://github.com/Romkabouter/Matrix-Voice-ESP32-MQTT-Audio-Streamer/blob/master/MatrixVoiceAudioServer/MatrixVoiceAudioServer.ino