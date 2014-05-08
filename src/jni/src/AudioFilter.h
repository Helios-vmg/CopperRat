#ifndef RESAMPLING_FILTER_H
#define RESAMPLING_FILTER_H
#include "Decoder.h"
#include <vector>

class AudioFilter;

class AudioFilterManager{
	bool need_two_buffers;
	audio_buffer_t saved_buffer;
	Decoder &decoder;
	bool filter_allocated;
	std::vector<AudioFilter *> filters;
	AudioFormat dst_format;
	bool dont_convert;

	void allocate_filters();
public:
	AudioFilterManager(Decoder &decoder, const AudioFormat &dst_format);
	~AudioFilterManager();
	audio_buffer_t read(audio_position_t position, memory_sample_count_t &samples_read_from_decoder);
};

#endif
