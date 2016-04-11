LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
ifeq "$(TARGET_ARCH_ABI)" "armeabi-v7a-hard"
	LOCAL_SRC_FILES := c:/users/josh/desktop/android/freetype-2.5.3-armeabi-v7a-hard/objs/freetype.a
else ifeq "$(TARGET_ARCH_ABI)" "x86"
	LOCAL_SRC_FILES := c:/users/josh/desktop/android/freetype-2.5.3-x86/objs/freetype.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ogg-vorbis
ifeq "$(TARGET_ARCH_ABI)" "armeabi-v7a-hard"
	LOCAL_SRC_FILES := c:/users/josh/desktop/android/libogg-1.3.2-armeabi-v7a-hard/src/ogg-vorbis.a
else ifeq "$(TARGET_ARCH_ABI)" "x86"
	LOCAL_SRC_FILES := c:/users/josh/desktop/android/libogg-1.3.2-x86/src/ogg-vorbis.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := main
LOCAL_C_INCLUDES := c:/users/josh/desktop/android/freetype-2.5.3-armeabi-v7a-hard/include
LOCAL_C_INCLUDES += c:/Users/Josh/Desktop/Android/libogg-1.3.2-armeabi-v7a-hard/include
LOCAL_C_INCLUDES += c:/Users/Josh/Desktop/Android/libvorbis-1.3.4-armeabi-v7a-hard/include
LOCAL_SRC_FILES := glesutil.c main.c core.c object.c utility.c menu.c shader.c
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2 -lz -lOpenSLES
LOCAL_CFLAGS := -std=c99 -O2
LOCAL_STATIC_LIBRARIES := android_native_app_glue freetype ogg-vorbis
include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)