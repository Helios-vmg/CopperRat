LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := tremolo
LOCAL_ARM_MODE   := arm
                    #bitwiseARM.s \
                    #dpenARM.s    \
                    #floor1ARM.s  \
                    #floor1LARM.s \
                    #mdctARM.s    \
                    #mdctLARM.s   \

LOCAL_SRC_FILES  := \
                    bitwise.c      \
                    codebook.c     \
                    dsp.c          \
                    floor0.c       \
                    floor1.c       \
                    floor_lookup.c \
                    framing.c      \
                    info.c         \
                    mapping0.c     \
                    mdct.c         \
                    misc.c         \
                    res012.c       \
                    vorbisfile.c   \

LOCAL_CFLAGS     := -DLITTLE_ENDIAN=1 -DBYTE_ORDER=1 #-D_ARM_ASSEM_

include $(BUILD_STATIC_LIBRARY)

