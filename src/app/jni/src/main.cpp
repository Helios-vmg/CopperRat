/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioPlayer.h"
#include "SUI/SUI.h"
#include "CommonFunctions.h"
#include "File.h"
#include "ApplicationState.h"
#include <memory>
#include <locale.h>
#include <jni.h>

std::unique_ptr<AudioPlayer> player;

extern "C" JNIEXPORT void JNICALL init_settings(JNIEnv *env, jclass cls){
	application_state = std::make_unique<ApplicationState>(false);
}

extern "C" JNIEXPORT void JNICALL destroy_settings(JNIEnv *env, jclass cls){
	application_state.reset();
}

extern "C" JNIEXPORT void JNICALL init_player(JNIEnv *env, jclass cls){
	player = std::make_unique<AudioPlayer>();
}

extern "C" JNIEXPORT void JNICALL destroy_player(JNIEnv *env, jclass cls){
	player.reset();
}

extern "C" JNIEXPORT void JNICALL run_player(JNIEnv *env, jclass cls){
	player->loop();
}

extern "C" JNIEXPORT void JNICALL stop_player(JNIEnv *env, jclass cls){
	player->request_exit();
}

static JNINativeMethod CopperRat_functions[] = {
	{ "init_settings",    "()V", (void *)init_settings    },
	{ "destroy_settings", "()V", (void *)destroy_settings },
	{ "init_player",      "()V", (void *)init_player      },
	{ "destroy_player",   "()V", (void *)destroy_player   },
};

static JNINativeMethod PlayerService_functions[] = {
	{ "run_player",  "()V", (void *)run_player  },
	{ "stop_player", "()V", (void *)stop_player },
};

template <size_t N>
void register_methods(JNIEnv *env, const char *classname, JNINativeMethod (&methods)[N]){
	jclass clazz = env->FindClass(classname);
	if (!clazz || env->RegisterNatives(clazz, methods, N) < 0)
		__android_log_print(ANDROID_LOG_ERROR, "C++", "Failed to register methods of %s", classname);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved){
	JNIEnv *env = nullptr;
	if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK){
		__android_log_print(ANDROID_LOG_ERROR, "C++", "Failed to get JNI Env");
		return JNI_VERSION_1_4;
	}

	register_methods(env, "org/copper/rat/CopperRat", CopperRat_functions);
	register_methods(env, "org/copper/rat/PlayerService", PlayerService_functions);
	return JNI_VERSION_1_4;
}

int main(int argc, char **argv){
	setlocale(LC_ALL, "en.UTF-8");
	SDL_SetHintWithPriority(SDL_HINT_ANDROID_BLOCK_ON_PAUSE_PAUSEAUDIO, "0", SDL_HINT_OVERRIDE);
	SDL_Init(SDL_INIT_EVERYTHING);
	initialize_resources();
	try{
		SUI sui(*player);
		player->sui = &sui;
		player->initialize(false);
		sui.initialize();
		android_start_thread();
		sui.loop();
		__android_log_print(ANDROID_LOG_INFO, "C++main", "%s", "Terminating normally.\n");
	}catch (const std::exception &e){
		__android_log_print(ANDROID_LOG_DEBUG, "C++Exception", "%s", e.what());
	}
	SDL_Quit();
	return 0;
}
