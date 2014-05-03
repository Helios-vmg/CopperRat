#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H
#include "ResamplingFilter.h"
#include <memory>
//#define DUMP_OUTPUT

#ifdef DUMP_OUTPUT
#include <fstream>
#endif

class AudioStream{
	std::auto_ptr<Decoder> decoder;
	std::auto_ptr<AudioFilterManager> filter;
	unsigned frequency;
	unsigned channels;
	unsigned default_buffer_length;
	audio_position_t position;
#ifdef DUMP_OUTPUT
	std::ofstream test_file;
#endif
public:
	AudioStream(const char *filename, unsigned frequency, unsigned channels, unsigned default_buffer_length);
	audio_buffer_t read_new();
	void reset();
};

#endif
