#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include "Exception.h"

class AudioPlayer;

class DeviceInitializationException : public CR_Exception{
public:
	DeviceInitializationException(const std::string &desc): CR_Exception(desc){}
};

class AudioDevice{
	bool audio_is_open;
public:
	AudioDevice(AudioPlayer &);
	~AudioDevice();
	void close();
	void start_audio();
	void pause_audio();
};
#endif
