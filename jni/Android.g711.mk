LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
TARGET_PLATFORM := 'android-19'
LOCAL_MODULE	:= libg711
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
SOURCE_FILE_PATH := webrtc/modules/audio_coding/codecs/g711
LOCAL_C_INCLUDES:= $(LOCAL_PATH) \
	$(LOCAL_PATH)/modules/audio_coding/codecs/g711

LOCAL_SRC_FILES := $(LOCAL_PATH)/webrtc/modules/audio_coding/codecs/g711/audio_decoder_pcm.cc \
	$(LOCAL_PATH)/webrtc/modules/audio_coding/codecs/g711/audio_encoder_pcm.cc \
	$(LOCAL_PATH)/webrtc/modules/audio_coding/codecs/g711/g711_interface.c \
	$(LOCAL_PATH)/webrtc/modules/audio_coding/codecs/g711/g711.c

LOCAL_CFLAGS := -w -DUNIX -DLINUX -DANDROID -DWEBRTC_LINUX -DWEBRTC_THREAD_RR -DWEBRTC_CLOCK_TYPE_REALTIME -DWEBRTC_ANDROID

include $(BUILD_STATIC_LIBRARY)