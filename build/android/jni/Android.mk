LOCAL_PATH := $(call my-dir)
SOURCE_PATH := $(LOCAL_PATH)/../../../src/android/jni
COMMON_PATH := $(LOCAL_PATH)/../../..

include $(CLEAR_VARS)

LOCAL_MODULE    := wxAndroid
LOCAL_CFLAGS    := -D__ANDROID__
LOCAL_SRC_FILES := $(SOURCE_PATH)/wxjni.cpp\
	$(SOURCE_PATH)/app.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../include
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)