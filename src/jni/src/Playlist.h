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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "Metadata.h"

class PlayItem{
	std::string path;

};

class Playlist{
public:
	enum class PlaybackMode{
		SINGLE = 0,
		REPEAT_LIST = 1,
		REPEAT_TRACK = 2,
		COUNT = 3,
	};
private:
	int current_track;
	PlaybackMode mode;
	bool shuffle;
	std::vector<std::wstring> tracks;
	std::vector<int> shuffle_vector;

	bool at_null_position() const{
		return this->current_track < 0 || (size_t)this->current_track >= this->tracks.size();
	}
	void load_playlist(const std::wstring &path);
public:
	Playlist(): current_track(-1), mode(PlaybackMode::REPEAT_LIST), shuffle(0){}
	void clear();
	void set(const std::vector<std::wstring> &v){
		this->clear();
		this->insert(v, 0);
	}
	void insert(const std::vector<std::wstring> &, size_t position);
	void toggle_shuffle();
	bool get_shuffle() const{
		return this->shuffle;
	}
	bool get_current_track(std::wstring &dst);
	bool next();
	bool back();
	bool is_back_possible() const;
	PlaybackMode cycle_mode();
	PlaybackMode get_playback_mode() const{
		return this->mode;
	}
	void load(bool file, const std::wstring &path);
	void append(bool file, const std::wstring &path);
};

std::wstring to_string(Playlist::PlaybackMode);

#endif
