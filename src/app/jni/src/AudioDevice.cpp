/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioDevice.h"
#include "AudioPlayer.h"
#include "ApplicationState.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL.h>
#endif

AudioDevice::AudioDevice(AudioPlayer &player): player(player), audio_is_open(false){}

AudioDevice::~AudioDevice(){
	this->close_in_main();
}

void AudioDevice::open_in_main(){
	if (this->audio_is_open)
		return;
#if !defined PROFILING
	SDL_AudioSpec specs;
	specs.freq = 44100;
	specs.format = AUDIO_S16SYS;
	specs.channels = 2;
	specs.samples = 1024*4;
	specs.callback = AudioPlayer::AudioCallback;
	specs.userdata = &this->player;

	if (SDL_OpenAudio(&specs, 0) < 0){
		this->error_string = "Could not initialize audio device: ";
		this->error_string += SDL_GetError();
		return;
	}
	this->audio_is_open = true;
#endif
}

void AudioDevice::close_in_main(){
	if (this->audio_is_open){
		SDL_CloseAudio();
		this->audio_is_open = 0;
	}
}

void AudioDevice::open(){
	if (!this->audio_is_open){
		this->open_in_main();
		if (!this->audio_is_open)
			throw DeviceInitializationException(this->error_string);
	}
}

void AudioDevice::close(){
	application_state.save();
	if (this->audio_is_open)
		this->close_in_main();
}

#undef SDL_PauseAudio

void AudioDevice::start_audio(){
	this->open();
	SDL_PauseAudio(0);
}

void AudioDevice::pause_audio(){
	SDL_PauseAudio(1);
}
