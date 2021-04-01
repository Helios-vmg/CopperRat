/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "ApplicationState.h"
#include "Metadata.h"

class PlayItem{
	std::string path;

};

class Playlist{
	PlayerState *state = nullptr;
	int current_track = -1;
	PlaybackMode mode;
	bool shuffle;
	std::vector<std::wstring> tracks;
	std::vector<int> shuffle_vector;

	bool at_null_position() const{
		return this->current_track < 0 || (size_t)this->current_track >= this->tracks.size();
	}
	void load_playlist(const std::wstring &path);
	void save_state();
public:
	Playlist() = default;
	Playlist(PlayerState &);
	~Playlist();
	Playlist(const Playlist &) = delete;
	Playlist &operator=(const Playlist &) = delete;
	Playlist(Playlist &&other){
		*this = std::move(other);
	}
	Playlist &operator=(Playlist &&);
	void clear();
	void set(const std::vector<std::wstring> &v){
		this->clear();
		this->insert(v, 0);
	}
	void insert(const std::vector<std::wstring> &, size_t position);
	bool toggle_shuffle();
	bool get_shuffle() const{
		return this->shuffle;
	}
	bool get_current_track(std::wstring &dst);
	int get_current_track_index(){
		if (this->at_null_position())
			return -1;
		return this->current_track;
	}
	bool next(bool by_user = 0);
	bool back();
	bool is_back_possible() const;
	PlaybackMode cycle_mode();
	PlaybackMode get_playback_mode() const{
		return this->mode;
	}
	void load(bool file, const std::wstring &path);
	void append(bool file, const std::wstring &path);
};

std::wstring to_string(PlaybackMode);

#endif
