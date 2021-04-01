/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "Exception.h"
#include "Threads.h"

class AudioPlayer;

class DeviceInitializationException : public CR_Exception{
public:
	DeviceInitializationException(const std::string &desc): CR_Exception(desc){}
	CR_Exception *clone() const{
		return new DeviceInitializationException(*this);
	}
};

class AudioDevice{
	friend class InitializeAudioDevice;
	AudioPlayer &player;
	bool audio_is_open;
	std::string error_string;

	void open_in_main();
public:
	AudioDevice(AudioPlayer &);
	~AudioDevice();
	void open();
	void close();
	void close_in_main();
	void start_audio();
	void pause_audio();
	bool is_open() const{
		return this->audio_is_open;
	}
};

#define SDL_PauseAudio(_)
