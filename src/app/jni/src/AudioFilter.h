/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "Decoder.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#endif

class AudioFilter;

class AudioFilterManager{
	bool need_two_buffers;
	audio_buffer_t saved_buffer;
	Decoder &decoder;
	bool filter_allocated;
	std::vector<std::pair<int, AudioFilter *> > filters;
	AudioFormat dst_format;
	bool dont_convert;
	double multiplier;

	void allocate_filters(double multiplier);
public:
	AudioFilterManager(Decoder &decoder, const AudioFormat &dst_format, double multiplier = 1.0);
	~AudioFilterManager();
	audio_buffer_t read(memory_sample_count_t &samples_read_from_decoder);
	AudioFormat get_dst_format() const{
		return this->dst_format;
	}
	void add_multiplication_filter(double factor);
	void clear_saved_buffer(){
		this->need_two_buffers = true;
		this->saved_buffer.unref();
	}
};
