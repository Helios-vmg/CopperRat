#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

class AudioPlayer;

struct DeviceInitializationException{};

class AudioDevice{
public:
	AudioDevice(AudioPlayer &);
	void start_audio();
	void pause_audio();
};
#endif
