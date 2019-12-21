PHONO_DSP_STREAMER_ESP32
Phono Preamp DSP RIAA curve streamer for SONOS and other http HLS capable WiFi audio speakers

Initially designed to run on ESP32-LyraT hardware using esp-idf libs ( Espressif IoT Development Framework ) and esp-adf libs.

DEV BOARD https://www.espressif.com/en/products/hardware/esp32-lyrat

ESP-ADF  https://github.com/espressif/esp-adf


DSP biquad IIR EQ working!

LINE LEVEL input to HeadPhone output PASSTHROUGH


warning, use ESP-ADF and ONLY compatible ESP-IDF. instructions on ESP-ADF github


PHASE 1 testing / notes


rename equalizer_hack.c to equalizer.c and replace in ESP-ADF framework.

remove equalizer_hack.c from main folder.


PHASE 2

1, test with technics SL-15 record deck.

2, add digital controlled preamp in from of codec if required for proto.

3, sine wave sweep / pink noise and check filter response


PHASE 3.

fire up wifi and push stream to laptop in 48khz 16bit pcm

try a few other compressions ( aac opus etc )

No obvious http HLS streaming server example in esp-adf libs. Needs investigating.

would it make more sense to push stream to SONOS server?


notes:

Matrix-Voice-ESP32-MQTT-Audio-Streamer ( its a slightly different dev board but its ESP32 so same environment.

https://github.com/Romkabouter/Matrix-Voice-ESP32-MQTT-Audio-Streamer/blob/master/MatrixVoiceAudioServer/MatrixVoiceAudioServer.ino

esp-dsp libs. https://github.com/espressif/esp-dsp
