PHONO_DSP_STREAMER_ESP32

EQ preamp streamer

HARDWARE

ESP32-LyraT 4.3


Code depends on

ESP-ADF

https://github.com/espressif/esp-adf

ESP-IDF

compatible IDF at time of writing, check ADF github for current compatible version of ESP-IDF

https://docs.espressif.com/projects/esp-idf/en/v3.3.1/versions.html

ESP-DSP

https://github.com/espressif/esp-dsp

download ESP-DSP to project directory recursively ( follow instruction )



DSP biquad IIR EQ working!

use vol up / down to change LPF frequency

use REC / MODE button to change Q factor

uncomment DSP_setup_fixedBiquad(b0,b1,b2,a1,a2); for phono RIIA curve


PHASE 1 testing / notes

some crackling caused by not oversampling ?
The DSP IIR filter processes with a delay line of 2.

I should sample at twice needed sample rate to get rid of noise ??
Needs to fix this before moving on to phase 2!


PHASE 2

1, test with record deck.

2, add digital controlled preamp in from of codec if required for proto.

3, sine wave sweep / pink noise and check filter response


PHASE 3.

fire up wifi and push stream to laptop in 48khz 16bit pcm ?

NOTE:

No obvious http HLS streaming server example in esp-adf libs. is it possible? I dought it !

Matrix-Voice-ESP32-MQTT-Audio-Streamer ( its a slightly different dev board but its ESP32 so same environment ?

https://github.com/Romkabouter/Matrix-Voice-ESP32-MQTT-Audio-Streamer/blob/master/MatrixVoiceAudioServer/MatrixVoiceAudioServer.ino

