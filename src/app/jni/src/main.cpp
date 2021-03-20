/*

Copyright (c) 2014, Helios
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "stdafx.h"
#include "AudioPlayer.h"
#include "SUI/SUI.h"
#include "CommonFunctions.h"
#include "File.h"
#include <locale.h>
#include <jni.h>

extern "C" JNIEXPORT jlong JNICALL init_player(JNIEnv *env, jclass cls){
	return (jlong)(intptr_t)new AudioPlayer();
}

extern "C" JNIEXPORT void JNICALL run_player(JNIEnv *env, jclass cls, jlong lplayer){
	auto player = (AudioPlayer *)(intptr_t)lplayer;
	player->loop();
}

extern "C" JNIEXPORT void JNICALL stop_player(JNIEnv *env, jclass cls, jlong lplayer){
	auto player = (AudioPlayer *)(intptr_t)lplayer;
	player->request_exit();
}

static JNINativeMethod PlayerService_functions[] = {
		{ "init_player", "()J",  (void *)init_player },
		{ "run_player",  "(J)V", (void *)run_player },
		{ "stop_player", "(J)V", (void *)stop_player },
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

	register_methods(env, "org/copper/rat/PlayerService", PlayerService_functions);
	return JNI_VERSION_1_4;
}

int main(int argc, char **argv){
	setlocale(LC_ALL, "en.UTF-8");
	SDL_Init(SDL_INIT_EVERYTHING);
	initialize_resources();
	try{
		SUI sui(*(AudioPlayer *)android_get_player());
		sui.loop();
		__android_log_print(ANDROID_LOG_INFO, "C++main", "%s", "Terminating normally.\n");
	}catch (const std::exception &e){
		__android_log_print(ANDROID_LOG_DEBUG, "C++Exception", "%s", e.what());
	}
	SDL_Quit();
	return 0;
}
