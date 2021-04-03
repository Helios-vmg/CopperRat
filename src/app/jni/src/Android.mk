LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_CFLAGS   += -O3 -DSDL_GPU_DISABLE_OPENGL -DGLEW_NO_GLU -DSDL_GPU_DISABLE_GLES_1 -DSDL_GPU_DISABLE_GLES_3 -fPIC -DBYTE_ORDER=1 -DLITTLE_ENDIAN=1 #-DPROFILING
LOCAL_CXXFLAGS += -O3 -std=c++17 -fexceptions -frtti "-DBOOST_NOINLINE=" -DSDL_GPU_DISABLE_OPENGL -fPIC #-DPROFILING
LOCAL_CXXFLAGS += -Wno-bitwise-op-parentheses -Wno-logical-op-parentheses -Wno-switch

LOCAL_C_INCLUDES := $(LOCAL_PATH)/SDL_gpu/
LOCAL_C_INCLUDES := $(LOCAL_PATH)/tremor/
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libogg-1.3.1/include/
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvorbis-1.3.4/include/
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvorbis-1.3.4/lib/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../flac-1.3.0/src/libFLAC/include/private/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../flac-1.3.0/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libmpg123/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../SDL2_image-2.0.0/

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/ApplicationState.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioBuffer.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioDevice.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioPlayer.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioPlayerState.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioStream.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/base64.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/BitShiftingFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/ChannelMixingFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/CommonFunctions.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Decoder.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/File.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/FLACDecoder.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Image.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/main.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Metadata.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Mp3Decoder.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/MultiplicationFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/OggDecoder.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Playlist.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/QueueElements.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/ResamplingFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SignednessFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Threads.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/XorShift128.cpp

LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/AlbumArt.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/Button.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/FileBrowser.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/Font.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/ListView.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/MainScreen.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/SUI.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/SeekBar.cpp

LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/glew.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/renderer_GLES_2.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/SDL_gpu.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/SDL_gpu_matrix.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/SDL_gpu_renderer.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/SDL_gpu_shapes.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/stb_image.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/stb_image_write.c

LOCAL_SRC_FILES += $(LOCAL_PATH)/../libogg-1.3.1/src/bitwise.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/../libogg-1.3.1/src/framing.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/block.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/codebook.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/floor0.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/floor1.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/info.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/mapping0.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/mdct.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/registry.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/res012.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/sharedbook.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/synthesis.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/vorbisfile.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/tremor/window.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_STATIC_LIBRARIES := libflacpp libmpg123 SDL2_image

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog
#LOCAL_LDLIBS += -Wl,--no-warn-shared-textrel

include $(BUILD_SHARED_LIBRARY)
