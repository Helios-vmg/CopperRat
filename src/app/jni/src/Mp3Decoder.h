/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

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
