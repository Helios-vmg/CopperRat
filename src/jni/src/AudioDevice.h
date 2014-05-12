#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#define DEFAULT_BUFFER_SIZE 4096

class AudioPlayer;

struct DeviceInitializationException{};

class AudioDevice{
public:
	AudioDevice(AudioPlayer &);
	void start_audio();
	void pause_audio();
};
#endif
