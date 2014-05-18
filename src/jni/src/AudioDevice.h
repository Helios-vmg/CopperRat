#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

class AudioPlayer;

struct DeviceInitializationException{};

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
