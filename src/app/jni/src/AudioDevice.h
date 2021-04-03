/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "Exception.h"
#include "Threads.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <optional>
#endif

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
	bool playing = false;
	std::optional<Uint32> last_active;

	void open_in_main();
	void close_in_main();
public:
	AudioDevice(AudioPlayer &);
	~AudioDevice();
	void open();
	void close();
	void start_audio();
	void pause_audio();
	bool is_open() const{
		return this->audio_is_open;
	}
	bool is_playing() const{
		return this->playing;
	}
	bool update(Uint32);
	
	class AudioLocker{
		bool restore;
		AudioDevice &dev;
	public:
		AudioLocker(AudioDevice &dev): dev(dev){
			this->restore = dev.is_open() && dev.playing;
			dev.pause_audio();
		}
		~AudioLocker(){
			if (this->restore)
				this->dev.start_audio();
		}
	};

};

#define SDL_PauseAudio(_)
