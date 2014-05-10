#ifndef FLACDECODER_H
#define FLACDECODER_H
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <FLAC++/decoder.h>
#include <fstream>
#include <queue>
#include "Decoder.h"

class FlacException : public DecoderException{
	std::string status_string;
public:
	FlacException(const char *s): status_string(s){}
	const std::string &what(){
		return this->status_string;
	}
};

class FlacDecoder: public Decoder, public FLAC::Decoder::Stream{
	std::ifstream file;
	std::deque<audio_buffer_t> buffers;
	typedef audio_buffer_t (*allocator_func)(const FLAC__Frame *, const FLAC__int32 * const *);
	static allocator_func allocator_functions[];
	AudioFormat declared_af;

	FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame *frame,const FLAC__int32 * const *buffer);
	FLAC__StreamDecoderReadStatus read_callback(FLAC__byte *buffer,size_t *bytes);
	FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
	bool eof_callback();
	FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
	FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
	void error_callback(::FLAC__StreamDecoderErrorStatus status){
		throw FlacException(FLAC__StreamDecoderErrorStatusString[status]);
	}
	
	audio_buffer_t read_more_internal();

	void free_buffers();

public:
	FlacDecoder(const char *filename);
	~FlacDecoder(){
		this->free_buffers();
	}
	AudioFormat get_audio_format(){
		return this->declared_af;
	}
	bool seek(audio_position_t);
	bool lazy_filter_allocation(){
		return 1;
	}
};

#endif
