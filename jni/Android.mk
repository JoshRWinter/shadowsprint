LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
ifeq "$(TARGET_ARCH_ABI)" "armeabi-v7a-hard"
	LOCAL_SRC_FILES := ../../freetype-2.5.3/objs/freetype-armeabi-v7a-hard.a
else ifeq "$(TARGET_ARCH_ABI)" "x86"
	LOCAL_SRC_FILES := ../../freetype-2.5.3/objs/freetype-x86.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ogg-vorbis
ifeq "$(TARGET_ARCH_ABI)" "armeabi-v7a-hard"
	LOCAL_SRC_FILES := ../../libogg-1.3.2/lib/ogg-vorbis-armeabi-v7a-hard.a
else ifeq "$(TARGET_ARCH_ABI)" "x86"
	LOCAL_SRC_FILES := ../../libogg-1.3.2/lib/ogg-vorbis-x86.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := main
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../freetype-2.5.3/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../libogg-1.3.2/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../libvorbis-1.3.4/include
LOCAL_SRC_FILES := glesutil.c main.c core.c object.c utility.c menu.c shader.c
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2 -lz -lOpenSLES
LOCAL_CFLAGS := -std=c99 -O2
LOCAL_STATIC_LIBRARIES := android_native_app_glue freetype ogg-vorbis
include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)
