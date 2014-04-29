#ifndef OGGDECODER_H
#define OGGDECODER_H
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <cstdio>
#include "Decoder.h"

class OggDecoder: public Decoder{
	FILE *file;
	int bitstream;
	OggVorbis_File ogg_file;
	unsigned frequency;
	unsigned channels;
	
	audio_buffer_t read_more();

public:
	OggDecoder(const char *filename);
	~OggDecoder();
	unsigned get_sampling_rate(){
		return this->frequency;
	}

	static size_t read(void *buffer, size_t size, size_t nmemb, void *s);
	static int seek(void *s, ogg_int64_t offset, int whence);
	static long tell(void *s);
};

#endif
