#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H
#include "AudioFilter.h"
#include <memory>
//#define DUMP_OUTPUT

#ifdef DUMP_OUTPUT
#include <fstream>
#endif

class AudioStream{
	std::auto_ptr<Decoder> decoder;
	std::auto_ptr<AudioFilterManager> filter;
	audio_position_t position;
#ifdef DUMP_OUTPUT
	std::ofstream test_file;
#endif
public:
	AudioStream(const char *filename, unsigned frequency, unsigned channels);
	audio_buffer_t read_new();
	void reset();
	audio_position_t seek(audio_position_t current_position, float ms);
};

#endif
