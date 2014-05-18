#include "AudioDevice.h"
#include "AudioPlayer.h"
#include <SDL.h>


AudioDevice::AudioDevice(AudioPlayer &player){
#ifndef PROFILING
	SDL_AudioSpec specs;
	specs.freq = 44100;
	specs.format = AUDIO_S16SYS;
	specs.channels = 2;
	specs.samples = 1024*4;
	specs.callback = AudioPlayer::AudioCallback;
	specs.userdata = &player;
	if (SDL_OpenAudio(&specs, 0) < 0)
		throw DeviceInitializationException();
	this->audio_is_open = 1;
#endif
}

AudioDevice::~AudioDevice(){
	this->close();
}

void AudioDevice::close(){
	if (this->audio_is_open)
		SDL_CloseAudio();
}

void AudioDevice::start_audio(){
	SDL_PauseAudio(0);
}

void AudioDevice::pause_audio(){
	SDL_PauseAudio(1);
}
