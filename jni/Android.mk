LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype-armeabi-v7a-hard
LOCAL_SRC_FILES := c:/users/josh/desktop/android/freetype-2.5.3-armeabi-v7a-hard/objs/freetype.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ogg-vorbis-armeabi-v7a-hard
LOCAL_SRC_FILES := c:/users/josh/desktop/android/libogg-1.3.2-armeabi-v7a-hard/src/ogg-vorbis.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := main
LOCAL_C_INCLUDES := c:/users/josh/desktop/android/freetype-2.5.3-armeabi-v7a-hard/include
LOCAL_C_INCLUDES += c:/Users/Josh/Desktop/Android/libogg-1.3.2-armeabi-v7a-hard/include
LOCAL_C_INCLUDES += c:/Users/Josh/Desktop/Android/libvorbis-1.3.4-armeabi-v7a-hard/include
LOCAL_SRC_FILES := ../../glesutil/glesutil.c main.c core.c object.c utility.c menu.c shader.c
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2 -lz -lOpenSLES
LOCAL_CFLAGS := -std=c99 -O2
LOCAL_STATIC_LIBRARIES := android_native_app_glue freetype-armeabi-v7a-hard ogg-vorbis-armeabi-v7a-hard
include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)