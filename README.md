PHONO_DSP_STREAMER_ESP32
Phono Preamp DSP RIAA curve streamer for SONOS and other http HLS capable WiFi audio speakers

Initially designed to run on ESP32-LyraT hardware using esp-idf libs ( Espressif IoT Development Framework ) and esp-adf libs.

https://www.espressif.com/en/products/hardware/esp32-lyrat

esp-adf libraries https://github.com/espressif/esp-adf

The project will be modified to support better ADC and DAC hardware if DSP RIAA curve in DSP IIR biquads is succesful.


PHASE 1 testing / notes

merge passthrough example with equalizer example adding equalizer to the audio pipe signal chain.


if equalizer is not powerful enough or cannot be modified ( max -13db gains on each frequency band, phono curve requires max attenuation of -37db @ 20khz ) then hack dsp code into the samples modifier code in equalizer.

dsps_iir_main.c DSP concept with esp-dsp libs. https://github.com/espressif/esp-dsp

notes:
float coeffs_lpf[5]; // load known biquad coefficiants here rather than generate them in code ?

float w_lpf[5] = {0,0}; // we don't need delay for biquad filter ?

float coeffs_lpf[0] = 0.105263157894737;

float coeffs_lpf[1] = -0.076417573949578;

float coeffs_lpf[2] = -0.024632736829188;

fwoat coeffs_lpf[3] = 1.866859545059558;

float coeffs_lpf[4] = -0.867284262601157;



// process samples in array with DSP IIR biquad RIIA phono curve
dsps_biquad_f32(sampleArrayIN, sampleArrayOUT, Nsamples, *coeffs_lpf, *w_lpf);



PHASE 2.

No obvious http HLS streaming server example in esp-adf libs. Needs investigating.

would it make more sense to push stream to SONOS server?

notes:

Matrix-Voice-ESP32-MQTT-Audio-Streamer ( its a slightly different dev board but its ESP32 so same environment.

https://github.com/Romkabouter/Matrix-Voice-ESP32-MQTT-Audio-Streamer/blob/master/MatrixVoiceAudioServer/MatrixVoiceAudioServer.ino
