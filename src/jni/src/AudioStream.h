#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H
#include "AudioFilter.h"
#include <memory>
//#define DUMP_OUTPUT

#ifdef DUMP_OUTPUT
#include <fstream>
#endif

class AudioPlayer;

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
	void seek(AudioPlayer *player, audio_position_t &new_position, audio_position_t current_position, double seconds);
	double get_total_time(){
		return this->decoder->get_seconds_length();
	}
};

#endif
