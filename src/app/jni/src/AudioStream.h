/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H
#include "AudioFilter.h"
//#define DUMP_OUTPUT
#ifndef HAVE_PRECOMPILED_HEADERS
#include <memory>
#include <memory>
#endif

#ifdef DUMP_OUTPUT
#include <fstream>
#endif

class AudioPlayerState;

class AudioStream{
	AudioPlayerState *parent;
	std::unique_ptr<Decoder> decoder;
	std::unique_ptr<AudioFilterManager> filter;
	audio_position_t position;
	std::wstring path;
#ifdef DUMP_OUTPUT
	std::ofstream test_file;
#endif
	AudioFormat dst_format;
	double multiplier;
public:
	AudioStream(AudioPlayerState &parent, const std::wstring &path, unsigned frequency, unsigned channels);
	audio_buffer_t read();
	bool reset();
	void seek(audio_position_t &new_position, audio_position_t current_position, double param, bool scaling);
	void seek(audio_position_t &new_position, audio_position_t current_position, double seconds);
	double get_total_time(){
		return this->decoder->get_seconds_length();
	}
	void metadata_update(std::shared_ptr<GenericMetadata>);
	AudioFormat get_stream_format() const{
		return this->decoder->get_audio_format();
	}
	AudioFormat get_preferred_format() const{
		return this->dst_format;
	}
	const std::wstring &get_path() const{
		return this->path;
	}
};

#endif
