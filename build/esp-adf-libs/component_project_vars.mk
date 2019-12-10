# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += /c/esp/esp-adf/components/esp-adf-libs/esp_audio/include /c/esp/esp-adf/components/esp-adf-libs/esp_codec/include/codec /c/esp/esp-adf/components/esp-adf-libs/esp_codec/include/processing /c/esp/esp-adf/components/esp-adf-libs/recorder_engine/include /c/esp/esp-adf/components/esp-adf-libs/esp_ssdp/include /c/esp/esp-adf/components/esp-adf-libs/esp_dlna/include /c/esp/esp-adf/components/esp-adf-libs/esp_upnp/include /c/esp/esp-adf/components/esp-adf-libs/esp_sip/include /c/esp/esp-adf/components/esp-adf-libs/audio_misc/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp-adf-libs -lesp-adf-libs -L/c/esp/esp-adf/components/esp-adf-libs/esp_audio/lib -L/c/esp/esp-adf/components/esp-adf-libs/esp_codec/lib -L/c/esp/esp-adf/components/esp-adf-libs/recorder_engine/lib -L/c/esp/esp-adf/components/esp-adf-libs/esp_ssdp/lib -L/c/esp/esp-adf/components/esp-adf-libs/esp_upnp/lib -L/c/esp/esp-adf/components/esp-adf-libs/esp_dlna/lib -L/c/esp/esp-adf/components/esp-adf-libs/esp_sip/lib -lesp_processing -lesp_audio -lesp-amr -lesp-amrwbenc -lesp-aac -lesp-ogg-container -lesp-opus -lesp-tremor -lesp-flac -lesp_ssdp -lesp_upnp -lesp_dlna -lesp_sip -lesp-mp3 -lrecorder_engine 
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esp-adf-libs
component-esp-adf-libs-build: 
