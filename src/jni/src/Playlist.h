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
	size_t current_track;
	bool current_track_is_repeated;
	PlaybackMode mode;
	bool shuffle;
	std::vector<std::wstring> tracks;
	std::vector<size_t> shuffle_vector;
public:
	Playlist(): current_track(0), mode(PlaybackMode::REPEAT_LIST), shuffle(0), current_track_is_repeated(0){}
	void clear();
	void set(const std::vector<std::wstring> &v){
		this->clear();
		this->insert(v, 0);
	}
	void insert(const std::vector<std::wstring> &, size_t position);
	void toggle_shuffle();
	bool pop(std::wstring &);
	bool back(std::wstring &);
	bool is_back_possible() const;
	PlaybackMode cycle_mode();
};

#endif
