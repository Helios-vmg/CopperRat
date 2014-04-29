#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H
#include "ResamplingFilter.h"

class AudioStream{
	Decoder *decoder;
	ResamplingFilter *filter;
	unsigned frequency;
	unsigned channels;
	unsigned default_buffer_length;
	audio_position_t position;
public:
	AudioStream(const char *filename, unsigned frequency, unsigned channels, unsigned default_buffer_length);
	~AudioStream();
	audio_buffer_t read_new();
	void reset();
};

#endif
