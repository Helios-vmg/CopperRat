#include "Threads.h"
#include "Playlist.h"

class Settings{
	typedef Playlist::PlaybackMode Mode;
	Mutex mutex;
	Mode playback_mode;
	bool shuffle;
	std::wstring last_browse_directory;

	void set_default_values();
public:
	Settings();
	~Settings(){
		this->commit();
	}
	void commit();
	//Setters:
	void set_playback_mode(Mode mode);
	void set_shuffle(bool shuffle);
	void set_last_browse_directory(const std::wstring &last_browse_directory);
	//Getters:
	Mode get_playback_mode();
	bool get_shuffle();
	std::wstring get_last_browse_directory();
};

extern Settings application_settings;
