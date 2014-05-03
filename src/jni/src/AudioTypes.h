#ifndef AUDIOTYPES_H
#define AUDIOTYPES_H
#include <SDL.h>

typedef Sint32 stereo_sample_t;
typedef Uint32 memory_sample_count_t;
typedef Uint64 sample_count_t;
typedef Uint32 memory_audio_position_t;
typedef Uint64 audio_position_t;

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

#if defined _MSC_VER || defined __GNUC__
#pragma pack(pop)
#endif

struct AudioFormat{
	unsigned channels;
	unsigned bytes_per_channel;
	unsigned freq;
};

#endif
