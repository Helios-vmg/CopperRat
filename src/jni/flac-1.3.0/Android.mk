LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libflacpp

#LOCAL_CFLAGS   += -O3 -DFLAC__NO_DLL -DFLAC__NO_ASM -DFLAC__HAS_OGG -DVERSION="\"1.3.0\"" -DFLaC__INLINE="_inline"
LOCAL_CFLAGS   += -O3 -DFLAC__NO_DLL -DFLAC__NO_ASM -DVERSION="\"1.3.0\"" -DFLaC__INLINE="_inline"
#LOCAL_CXXFLAGS += -O3 -std=c++0x -DFLAC__NO_DLL -DFLAC__NO_ASM -DFLAC__HAS_OGG -DVERSION="\"1.3.0\"" -DFLaC__INLINE="_inline"
LOCAL_CXXFLAGS += -O3 -std=c++0x -frtti -DFLAC__NO_DLL -DFLAC__NO_ASM -DVERSION="\"1.3.0\"" -DFLaC__INLINE="_inline"

LOCAL_C_INCLUDES += $(LOCAL_PATH)/libFLAC/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include/

LOCAL_SRC_FILES += libFLAC/bitmath.c
LOCAL_SRC_FILES += libFLAC/bitreader.c
LOCAL_SRC_FILES += libFLAC/bitwriter.c
LOCAL_SRC_FILES += libFLAC/cpu.c
LOCAL_SRC_FILES += libFLAC/crc.c
LOCAL_SRC_FILES += libFLAC/fixed.c
LOCAL_SRC_FILES += libFLAC/float.c
LOCAL_SRC_FILES += libFLAC/format.c
LOCAL_SRC_FILES += libFLAC/lpc.c
LOCAL_SRC_FILES += libFLAC/md5.c
LOCAL_SRC_FILES += libFLAC/memory.c
LOCAL_SRC_FILES += libFLAC/metadata_iterators.c
LOCAL_SRC_FILES += libFLAC/metadata_object.c
LOCAL_SRC_FILES += libFLAC/ogg_decoder_aspect.c
LOCAL_SRC_FILES += libFLAC/ogg_encoder_aspect.c
LOCAL_SRC_FILES += libFLAC/ogg_helper.c
LOCAL_SRC_FILES += libFLAC/ogg_mapping.c
LOCAL_SRC_FILES += libFLAC/stream_decoder.c
LOCAL_SRC_FILES += libFLAC/stream_encoder.c
LOCAL_SRC_FILES += libFLAC/stream_encoder_framing.c
LOCAL_SRC_FILES += libFLAC/window.c
LOCAL_SRC_FILES += libFLAC++/metadata.cpp
LOCAL_SRC_FILES += libFLAC++/stream_decoder.cpp
LOCAL_SRC_FILES += libFLAC++/stream_encoder.cpp

include $(BUILD_STATIC_LIBRARY)
