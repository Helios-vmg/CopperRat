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

#ifndef DECODER_H
#define DECODER_H
#include "AudioTypes.h"
#include "Metadata.h"
#include "AudioBuffer.h"
#include "Exception.h"
#include "CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <limits>
#endif

class AudioStream;

class Decoder{
	audio_position_t current_position;
	audio_position_t length;
	double seconds_length;
protected:
	AudioStream &parent;
	std::wstring path;
	//OggMetadata metadata;
	virtual audio_buffer_t read_more_internal() = 0;
	virtual sample_count_t get_pcm_length_internal() = 0;
	virtual double get_seconds_length_internal() = 0;
public:
	Decoder(AudioStream &parent, const std::wstring &path):
		parent(parent),
		//metadata(path),
		path(path),
		current_position(0),
		length(invalid_sample_count),
		seconds_length(-1){}
	virtual ~Decoder(){}
	virtual AudioFormat get_audio_format() const = 0;
	virtual bool lazy_filter_allocation(){
		return 0;
	}
	audio_buffer_t read();
	static Decoder *create(AudioStream &, const std::wstring &path);
	virtual bool seek(audio_position_t) = 0;
	virtual bool fast_seek(audio_position_t p, audio_position_t &new_position){
		bool ret = this->seek(p);
		if (ret)
			new_position = p;
		return ret;
	}
	virtual bool fast_seek_seconds(double p, audio_position_t &new_position){
		return 0;
	}
	sample_count_t get_pcm_length(){
		if (this->length != invalid_sample_count)
			return this->length;
		return this->length = this->get_pcm_length_internal();
	}
	double get_seconds_length(){
		if (this->seconds_length >= 0)
			return this->seconds_length;
		return this->seconds_length = this->get_seconds_length_internal();
	}
	virtual bool fast_seek_takes_seconds() const{
		return 0;
	}
};

class DecoderException : public CR_Exception{
public:
	DecoderException(const std::string &description): CR_Exception(description){}
	virtual ~DecoderException(){}
	virtual CR_Exception *clone() const{
		return new CR_Exception(*this);
	}
};

class DecoderInitializationException : public DecoderException{
public:
	DecoderInitializationException(const std::string &description): DecoderException(description){
	}
	virtual CR_Exception *clone() const{
		return new DecoderInitializationException(*this);
	}
};

class FileNotFoundException : public DecoderInitializationException{
public:
	FileNotFoundException(const std::string &description): DecoderInitializationException("File not found: " + description){
	}
	virtual CR_Exception *clone() const{
		return new FileNotFoundException(*this);
	}
};

void filter_list_by_supported_formats(std::vector<std::wstring> &dst, const std::vector<std::wstring> &src);
bool format_is_supported(const std::wstring &);

#endif
