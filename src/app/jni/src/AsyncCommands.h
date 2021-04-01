#pragma once

class AudioPlayerState;

class AudioPlayerAsyncCommand{
public:
	virtual ~AudioPlayerAsyncCommand(){}
	virtual bool execute(AudioPlayerState &) = 0;
};

#define DECLARE_ASYNC_COMMAND(name) class AsyncCommand##name : public AudioPlayerAsyncCommand{ \
public: \
	bool execute(AudioPlayerState &) override; \
}

DECLARE_ASYNC_COMMAND(HardPlay);
DECLARE_ASYNC_COMMAND(PlayPause);
DECLARE_ASYNC_COMMAND(Play);
DECLARE_ASYNC_COMMAND(Pause);
DECLARE_ASYNC_COMMAND(Stop);
DECLARE_ASYNC_COMMAND(Previous);
DECLARE_ASYNC_COMMAND(Next);
DECLARE_ASYNC_COMMAND(Exit);
DECLARE_ASYNC_COMMAND(PlaybackEnd);

class AsyncCommandAbsoluteSeek : public AudioPlayerAsyncCommand{
	bool scaling;
	double param;
public:
	AsyncCommandAbsoluteSeek(double param, bool scaling = 1): param(param), scaling(scaling){}
	bool execute(AudioPlayerState &) override;
};

class AsyncCommandRelativeSeek : public AudioPlayerAsyncCommand{
	double seconds;
public:
	AsyncCommandRelativeSeek(double seconds): seconds(seconds){}
	bool execute(AudioPlayerState &) override;
};

class AsyncCommandLoad : public AudioPlayerAsyncCommand{
	bool load,
		file;
	std::wstring path;
public:
	AsyncCommandLoad(bool load, bool file, const std::wstring &path): load(load), file(file), path(path){}
	bool execute(AudioPlayerState &) override;
};
