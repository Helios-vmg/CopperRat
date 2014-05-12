#include "AudioDevice.h"
#include "AudioPlayer.h"
#include <SDL.h>

AudioDevice::AudioDevice(AudioPlayer &player){
#ifndef PROFILING
	SDL_AudioSpec specs;
	specs.freq = 44100;
	specs.format = AUDIO_S16SYS;
	specs.channels = 2;
	specs.samples = DEFAULT_BUFFER_SIZE;
	specs.callback = AudioPlayer::AudioCallback;
	specs.userdata = &player;
	if (SDL_OpenAudio(&specs, 0) < 0)
		throw DeviceInitializationException();
#endif
}

void AudioDevice::start_audio(){
	SDL_PauseAudio(0);
}

void AudioDevice::pause_audio(){
	SDL_PauseAudio(1);
}
