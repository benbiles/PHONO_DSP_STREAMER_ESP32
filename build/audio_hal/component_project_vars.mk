# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += /c/esp/esp-adf/components/audio_hal/include /c/esp/esp-adf/components/audio_hal/driver/es8388 /c/esp/esp-adf/components/audio_hal/driver/es8374 /c/esp/esp-adf/components/audio_hal/driver/es8311 /c/esp/esp-adf/components/audio_hal/driver/es7243 /c/esp/esp-adf/components/audio_hal/driver/zl38063 /c/esp/esp-adf/components/audio_hal/driver/zl38063/api_lib /c/esp/esp-adf/components/audio_hal/driver/zl38063/example_apps /c/esp/esp-adf/components/audio_hal/driver/zl38063/firmware
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/audio_hal -laudio_hal -L/c/esp/esp-adf/components/audio_hal/driver/zl38063/firmware -lfirmware
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += audio_hal
component-audio_hal-build: 
