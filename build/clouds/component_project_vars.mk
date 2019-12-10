# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += /c/esp/esp-adf/components/clouds/include /c/esp/esp-adf/components/clouds/dueros/lightduer/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/clouds -lclouds -L/c/esp/esp-adf/components/clouds/dueros/lightduer -lduer-device
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += clouds
component-clouds-build: 
