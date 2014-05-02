#ifndef AUDIOTYPES_H
#define AUDIOTYPES_H
#include <SDL.h>

typedef Sint16 sample_t;
typedef Sint32 stereo_sample_t;
typedef Uint64 sample_count_t;
typedef Uint64 audio_position_t;

struct AudioFormat{
	unsigned channels;
	unsigned bytes_per_channel;
	unsigned freq;
};

#endif
