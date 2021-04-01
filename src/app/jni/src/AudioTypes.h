/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
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
