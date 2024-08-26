LOCAL_PATH := $(call my-dir)


# STATIC LIBRARY

include $(CLEAR_VARS)

MY_INCLUDE_FILES := $(LOCAL_PATH)
MY_CFLAGS := -DOPT_GENERIC

LOCAL_MODULE := libmpg123
LOCAL_C_INCLUDES := $(MY_INCLUDE_FILES)
LOCAL_CFLAGS := $(MY_CFLAGS)
LOCAL_ARM_MODE := arm

LOCAL_LDLIBS :=

LOCAL_SRC_FILES := \
  libmpg123.c \
  parse.c \
  frame.c \
  format.c \
  dct64.c \
  equalizer.c \
  id3.c \
  optimize.c \
  readers.c \
  tabinit.c \
  index.c \
  layer1.c \
  layer2.c \
  layer3.c \
  ntom.c \
  synth.c \
  stringbuf.c \
  icy2utf8.c \
  icy.c \
  synth_real.c \
  synth_8bit.c \
  synth_s32.c \
  compat.c \
  feature.c \

LOCAL_EXPORT_C_INCLUDES := $(MY_INCLUDE_FILES)
LOCAL_EXPORT_CFLAGS := $(LOCAL_CFLAGS)

include $(BUILD_STATIC_LIBRARY)
