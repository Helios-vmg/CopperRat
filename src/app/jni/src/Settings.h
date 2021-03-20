#ifndef SETTINGS_H
#define SETTINGS_H

#include "Threads.h"
#include "Playlist.h"

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

class Settings{
	typedef Playlist::PlaybackMode Mode;
	Mutex mutex;
	bool no_changes;
	Mode playback_mode;
	bool shuffle;
	std::wstring last_root;
	std::wstring last_browse_directory;
	int current_track;
	double current_time;
	std::vector<std::wstring> playlist_items;
	std::vector<int> shuffle_items;
	VisualizationMode visualization_mode;
	bool display_fps;

	void set_default_values();
public:
	Settings();
	~Settings(){
		this->commit();
	}
	void commit();
	//Setters:
	void set_playback_mode(Mode);
	void set_shuffle(bool);
	void set_last_root(const std::wstring &);
	void set_last_browse_directory(const std::wstring &);
	void set_current_track(int);
	void set_current_time(double);
	void set_playlist_items(const std::vector<std::wstring> &);
	void set_shuffle_items(const std::vector<int> &);
	void set_visualization_mode(VisualizationMode vm);
	void set_display_fps(bool);
	//Getters:
	Mode get_playback_mode();
	bool get_shuffle();
	std::wstring get_last_root();
	std::wstring get_last_browse_directory();
	int get_current_track();
	double get_current_time();
	void get_playlist_items(std::vector<std::wstring> &);
	void get_shuffle_items(std::vector<int> &);
	VisualizationMode get_visualization_mode();
	bool get_display_fps();
};

extern Settings application_settings;

#endif
