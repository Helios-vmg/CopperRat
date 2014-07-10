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

#ifndef AUDIOTYPES_H
#define AUDIOTYPES_H
#include <SDL.h>

typedef Sint32 stereo_sample_t;
typedef Uint32 memory_sample_count_t;
typedef Uint64 sample_count_t;
typedef Uint32 memory_audio_position_t;
typedef Uint64 audio_position_t;

const audio_position_t invalid_audio_position = 0xFFFFFFFFFFFFFFFF;
const sample_count_t invalid_sample_count = 0xFFFFFFFFFFFFFFFF;

#if defined _MSC_VER || defined __GNUC__
#pragma pack(push, 1)
#define PACKING_ATTRIBUTE
#elif defined __clang__
#define PACKING_ATTRIBUTE __attribute__((packed))
#endif

template <typename NumberT, unsigned Channels>
struct PACKING_ATTRIBUTE sample_t{
	NumberT values[Channels];
};

template <typename NumberT>
struct PACKING_ATTRIBUTE sample_t<NumberT, 0>{
	NumberT values;
};

#if defined _MSC_VER || defined __GNUC__
#pragma pack(pop)
#endif

struct AudioFormat{
	bool is_signed;
	unsigned bytes_per_channel;
	unsigned channels;
	unsigned freq;
	AudioFormat(){}
	AudioFormat(bool is_signed, unsigned bytes_per_channel, unsigned channels, unsigned freq):
		is_signed(is_signed),
		bytes_per_channel(bytes_per_channel),
		channels(channels),
		freq(freq){}
	unsigned bytes_per_sample() const{
		return this->bytes_per_channel * this->channels;
	}
};

#endif
