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
#include "AudioDevice.h"
#include "AudioPlayer.h"
#include "Settings.h"
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
	application_settings.commit();
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
