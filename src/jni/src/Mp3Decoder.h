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

#ifndef MP3DECODER_H
#define MP3DECODER_H
#include "Decoder.h"

class mp3_static_data{
	bool initialized;
public:
	mp3_static_data():initialized(0){}
	~mp3_static_data();
	bool init();
};

struct mpg123_handle_struct;

typedef struct mpg123_handle_struct mpg123_handle;

class Mp3Decoder : public Decoder{
	static mp3_static_data static_data;
	mpg123_handle *handle;
	int fd;
	bool has_played;
	AudioFormat format;
	sample_count_t length;
	double seconds_per_frame;
	void *id3v1;
	void *id3v2;
	bool metadata_done;
	
	audio_buffer_t read_more_internal();
	sample_count_t get_pcm_length_internal(){
		return this->length;
	}
	double get_seconds_length_internal();

	long freq;
	int channels_enum;
	int encoding_enum;

	void set_format();
	void check_for_metadata();

public:
	Mp3Decoder(AudioStream &parent, const std::wstring &path);
	~Mp3Decoder();
	AudioFormat get_audio_format() const{
		return this->format;
	}
	bool seek(audio_position_t);
	bool fast_seek(audio_position_t position, audio_position_t &new_position){
		return this->fast_seek_seconds((double)position / (double)this->freq, new_position);
	}
	bool fast_seek_seconds(double p, audio_position_t &new_position);
	bool fast_seek_takes_seconds() const{
		return 1;
	}
};

#endif
