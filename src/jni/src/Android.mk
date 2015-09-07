LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_CFLAGS   += -O3 -DSDL_GPU_DISABLE_OPENGL #-DPROFILING
LOCAL_CXXFLAGS += -O3 -std=gnu++11 -fexceptions "-DBOOST_NOINLINE=" -DSDL_GPU_DISABLE_OPENGL #-DPROFILING

LOCAL_C_INCLUDES := $(LOCAL_PATH)/SDL_gpu/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/SDL_gpu/externals/glew/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/SDL_gpu/externals/stb_image/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libogg-1.3.1/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvorbis-1.3.4/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Tremolo008/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libvorbis-1.3.4/lib/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../flac-1.3.0/src/libFLAC/include/private/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../flac-1.3.0/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libmpg123/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../SDL2_image-2.0.0/

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioBuffer.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioDevice.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioPlayer.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/AudioStream.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/BitShiftingFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/ChannelMixingFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/CommonFunctions.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Decoder.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/FLACDecoder.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/File.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Image.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Metadata.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Mp3Decoder.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/MultiplicationFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/OggDecoder.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Playlist.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/ResamplingFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Settings.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SignednessFilter.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/Threads.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/base64.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/main.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/boost/context/jump_arm_aapcs_elf_gas.S
LOCAL_SRC_FILES += $(LOCAL_PATH)/boost/context/make_arm_aapcs_elf_gas.S
LOCAL_SRC_FILES += $(LOCAL_PATH)/boost/coroutine/exceptions.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/boost/coroutine/detail/coroutine_context.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/boost/coroutine/posix/stack_traits.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/boost/thread/pthread/once_atomic.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/boost/system/error_code.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/tinyxml2.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/SDL_gpu.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/SDL_gpu_Renderer.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/SDL_gpu_shapes.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/SDL_gpu_matrix.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/renderer_GLES_1.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/renderer_GLES_2.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/externals/stb_image/stb_image.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SDL_gpu/externals/stb_image/stb_image_write.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/AlbumArt.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/Button.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/FileBrowser.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/Font.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/ListView.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/MainScreen.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/SUI.cpp
LOCAL_SRC_FILES += $(LOCAL_PATH)/SUI/SeekBar.cpp

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_STATIC_LIBRARIES := libflacpp tremolo libmpg123 SDL2_image

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
