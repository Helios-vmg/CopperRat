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
public:
	Playlist(): current_track(-1), mode(PlaybackMode::REPEAT_LIST), shuffle(0){}
	void clear();
	void set(const std::vector<std::wstring> &v){
		this->clear();
		this->insert(v, 0);
	}
	void insert(const std::vector<std::wstring> &, size_t position);
	void toggle_shuffle();
	bool get_current_track(std::wstring &dst);
	bool next();
	bool back();
	bool is_back_possible() const;
	PlaybackMode cycle_mode();
};

#endif
