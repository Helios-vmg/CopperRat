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

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H
#include "AudioFilter.h"
#include "auto_ptr.h"
//#define DUMP_OUTPUT
#ifndef HAVE_PRECOMPILED_HEADERS
#include <memory>
#include <boost/shared_ptr.hpp>
#endif

#ifdef DUMP_OUTPUT
#include <fstream>
#endif

class AudioPlayer;

class AudioStream{
	AudioPlayer &parent;
	CR_UNIQUE_PTR(Decoder) decoder;
	CR_UNIQUE_PTR(AudioFilterManager) filter;
	audio_position_t position;
	std::wstring path;
#ifdef DUMP_OUTPUT
	std::ofstream test_file;
#endif
	AudioFormat dst_format;
	double multiplier;
public:
	AudioStream(AudioPlayer &parent, const std::wstring &path, unsigned frequency, unsigned channels);
	audio_buffer_t read();
	bool reset();
	void seek(AudioPlayer *player, audio_position_t &new_position, audio_position_t current_position, double param, bool scaling);
	void seek(AudioPlayer *player, audio_position_t &new_position, audio_position_t current_position, double seconds);
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
