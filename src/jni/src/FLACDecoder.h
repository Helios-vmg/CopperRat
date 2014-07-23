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

#ifndef FLACDECODER_H
#define FLACDECODER_H

#include "Decoder.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <FLAC++/decoder.h>
#include <fstream>
#include <queue>
#endif

class FlacException : public DecoderException{
public:
	FlacException(const std::string &s): DecoderException(s){}
	virtual CR_Exception *clone() const{
		return new FlacException(*this);
	}
};

class FlacDecoder: public Decoder, public FLAC::Decoder::Stream{
	std::ifstream file;
	OggMetadata metadata;
	std::deque<audio_buffer_t> buffers;
	typedef audio_buffer_t (*allocator_func)(const FLAC__Frame *, const FLAC__int32 * const *);
	static allocator_func allocator_functions[];
	bool declared_af_set;
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
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	
	audio_buffer_t read_more_internal();
	sample_count_t get_pcm_length_internal();
	double get_seconds_length_internal();

	void free_buffers();
	void read_vorbis_comments(const FLAC__StreamMetadata_VorbisComment &comments);

public:
	FlacDecoder(AudioStream &parent, const std::wstring &path);
	~FlacDecoder(){
		this->free_buffers();
	}
	AudioFormat get_audio_format() const{
		return this->declared_af;
	}
	bool seek(audio_position_t);
	bool lazy_filter_allocation(){
		return 1;
	}
};

#endif
