#ifndef RESAMPLING_FILTER_H
#define RESAMPLING_FILTER_H
#include "Decoder.h"
#include <vector>

class AudioFilter;

class AudioFilterManager{
	Decoder &decoder;
	std::vector<AudioFilter *> filters;
public:
	AudioFilterManager(Decoder &decoder, const AudioFormat &dst_format);
	~AudioFilterManager();
	audio_buffer_t read(audio_position_t position, memory_sample_count_t &samples_read_from_decoder);
};

#endif
