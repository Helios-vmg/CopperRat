#ifndef OGGDECODER_H
#define OGGDECODER_H
#include "Decoder.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <cstdio>
#endif

class OggDecoder: public Decoder{
	FILE *file;
	int bitstream;
	OggVorbis_File ogg_file;
	unsigned frequency;
	unsigned channels;
	OggMetadata metadata;
	
	audio_buffer_t read_more_internal();
	sample_count_t get_pcm_length_internal();
	double get_seconds_length_internal();

public:
	OggDecoder(AudioStream &parent, const std::wstring &path);
	~OggDecoder();
	AudioFormat get_audio_format(){
		return AudioFormat(true, 2, this->channels, this->frequency);
	}
	bool seek(audio_position_t);
	bool fast_seek(audio_position_t p, audio_position_t &new_position);

	static size_t read(void *buffer, size_t size, size_t nmemb, void *s);
	static int seek(void *s, ogg_int64_t offset, int whence);
	static long tell(void *s);
};

#endif
