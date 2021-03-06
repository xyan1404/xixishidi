LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
TARGET_PLATFORM := 'android-19'
LOCAL_MODULE	:= libg722
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
SOURCE_FILE_PATH := webrtc/modules/audio_coding/codecs/g722
LOCAL_C_INCLUDES:= $(LOCAL_PATH) \
	$(LOCAL_PATH)/modules/audio_coding/codecs/g711

LOCAL_SRC_FILES := $(LOCAL_PATH)/$(SOURCE_FILE_PATH)/audio_decoder_g722.cc \
	$(LOCAL_PATH)/$(SOURCE_FILE_PATH)/audio_encoder_g722.cc \
	$(LOCAL_PATH)/$(SOURCE_FILE_PATH)/g722_decode.c \
	$(LOCAL_PATH)/$(SOURCE_FILE_PATH)/g722_encode.c \
	$(LOCAL_PATH)/$(SOURCE_FILE_PATH)/g722_decode.c \
	$(LOCAL_PATH)/$(SOURCE_FILE_PATH)/g722_interface.c

LOCAL_CFLAGS := -w -DUNIX -DLINUX -DANDROID -DWEBRTC_LINUX -DWEBRTC_THREAD_RR -DWEBRTC_CLOCK_TYPE_REALTIME -DWEBRTC_ANDROID

include $(BUILD_STATIC_LIBRARY)