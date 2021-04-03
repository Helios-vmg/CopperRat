/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "Threads.h"
#include <mutex>
#include <map>

enum class VisualizationMode{
	NONE = 0,
	OSCILLOSCOPE,
	SPECTRUM_LOW,
	SPECTRUM_MID,
	SPECTRUM_HIGH,
	SPECTRUM_MAX,
	END,
};

std::wstring to_string(VisualizationMode mode);

enum class PlaybackMode{
	SINGLE = 0,
	REPEAT_LIST = 1,
	REPEAT_TRACK = 2,
	COUNT = 3,
};

class PlaybackState{
	typedef PlaybackMode Mode;
	mutable Mutex mutex;
	mutable bool no_changes = true;
	int index = -1;
	Mode playback_mode = Mode::REPEAT_LIST;
	bool shuffle = false;
	int current_track = -1;
	double current_time = -1;
	
	std::string get_path() const;
public:
	PlaybackState() = default;
	PlaybackState(int index, bool force_new);
	~PlaybackState(){
		this->save();
	}
	PlaybackState(const PlaybackState &) = delete;
	PlaybackState &operator=(const PlaybackState &) = delete;
	PlaybackState(PlaybackState &&other){
		*this = std::move(other);
	}
	PlaybackState &operator=(PlaybackState &&other);
	void save() const;
	
	//Setters:
	void set_playback_mode(Mode);
	void set_shuffle(bool);
	void set_current_track(int);
	void set_current_time(double);
	
	//Getters:
	int get_index() const{
		return this->index;
	}
	Mode get_playback_mode() const;
	bool get_shuffle() const;
	int get_current_track() const;
	double get_current_time() const;
};

class PlaylistState{
	mutable Mutex mutex;
	mutable bool no_changes = true;
	int index = -1;
	std::vector<std::wstring> playlist_items;
	std::vector<int> shuffle_items;
	
	std::string get_path() const;
public:
	PlaylistState() = default;
	PlaylistState(int index, bool force_new);
	~PlaylistState(){
		this->save();
	}
	PlaylistState(const PlaylistState &) = delete;
	PlaylistState &operator=(const PlaylistState &) = delete;
	PlaylistState(PlaylistState &&other){
		*this = std::move(other);
	}
	PlaylistState &operator=(PlaylistState &&other);
	void save() const;
	
	//Setters:
	void set_playlist_items(const std::vector<std::wstring> &);
	void set_shuffle_items(const std::vector<int> &);
	
	//Getters:
	void get_playlist_items(std::vector<std::wstring> &) const;
	void get_shuffle_items(std::vector<int> &) const;
};

class PlayerState{
	bool read_only = false;
	PlaybackState playback;
	PlaylistState playlist;
public:
	PlayerState() = default;
	PlayerState(bool read_only, int index, bool force_new): read_only(read_only), playback(index, force_new), playlist(index, force_new){}
	~PlayerState(){
		this->save();
	}
	PlayerState(const PlayerState &) = delete;
	PlayerState &operator=(const PlayerState &) = delete;
	PlayerState(PlayerState &&other){
		*this = std::move(other);
	}
	PlayerState &operator=(PlayerState &&other){
		this->read_only = std::move(other.read_only);
		this->playback = std::move(other.playback);
		this->playlist = std::move(other.playlist);
		return *this;
	}
	
	void save() const;
	const PlaybackState &get_playback() const{
		return this->playback;
	}
	PlaybackState &get_playback(){
		return this->playback;
	}
	const PlaylistState &get_playlist() const{
		return this->playlist;
	}
	PlaylistState &get_playlist(){
		return this->playlist;
	}
};

class ApplicationState{
	const bool read_only;
	mutable std::mutex mutex;
	mutable bool no_changes = true;
	std::wstring last_root;
	std::wstring last_browse_directory;
	VisualizationMode visualization_mode;
	bool display_fps;
	std::map<int, PlayerState> players;
	int current_player = 0;

	void reset();
	void load_members();
public:
	ApplicationState(bool read_only = false);
	~ApplicationState(){
		this->save();
	}
	void save();
	std::unique_lock<std::mutex> lock() const{
		return std::unique_lock<std::mutex>(this->mutex);
	}
	PlayerState &new_player();
	void erase(PlayerState &);
	ApplicationState(const ApplicationState &) = delete;
	ApplicationState &operator=(const ApplicationState &) = delete;
	ApplicationState(ApplicationState &&) = delete;
	ApplicationState &operator=(ApplicationState &&) = delete;
	
	//Setters:
	void set_last_root(const std::wstring &);
	void set_last_browse_directory(const std::wstring &);
	void set_visualization_mode(VisualizationMode vm);
	void set_display_fps(bool);
	void set_current_player_index(int);
	
	//Getters:
	std::wstring get_last_root() const;
	std::wstring get_last_browse_directory() const;
	VisualizationMode get_visualization_mode() const;
	bool get_display_fps() const;
	const PlayerState &get_current_player() const;
	PlayerState &get_current_player();
	int get_current_player_index() const;
	const std::map<int, PlayerState> &get_players() const{
		return this->players;
	}
	std::map<int, PlayerState> &get_players(){
		return this->players;
	}
};

extern std::unique_ptr<ApplicationState> application_state;
