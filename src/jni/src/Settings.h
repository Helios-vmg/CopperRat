#include "Threads.h"
#include "Playlist.h"

class Settings{
	typedef Playlist::PlaybackMode Mode;
	Mutex mutex;
	Mode playback_mode;
	bool shuffle;
	std::wstring last_browse_directory;
	int current_track;
	double current_time;
	std::vector<std::wstring> playlist_items;
	std::vector<int> shuffle_items;

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
	void set_last_browse_directory(const std::wstring &);
	void set_current_track(int);
	void set_current_time(double);
	void set_playlist_items(const std::vector<std::wstring> &);
	void set_shuffle_items(const std::vector<int> &);
	//Getters:
	Mode get_playback_mode();
	bool get_shuffle();
	std::wstring get_last_browse_directory();
	int get_current_track();
	double get_current_time();
	void get_playlist_items(std::vector<std::wstring> &);
	void get_shuffle_items(std::vector<int> &);
};

extern Settings application_settings;
