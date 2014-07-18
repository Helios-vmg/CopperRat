/*

Copyright (c) 2014, Helios
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef RESAMPLING_FILTER_H
#define RESAMPLING_FILTER_H
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
};

#endif
