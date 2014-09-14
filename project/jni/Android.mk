LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := NativeTest
LOCAL_CFLAGS    := -std=c++14
LOCAL_SRC_FILES := NativeTest.cpp
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
