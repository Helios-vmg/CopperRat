#include "stdafx.h"
#include "AsyncCommands.h"
#include "AudioPlayer.h"

#define IMPLEMENT_ASYNC_COMMAND(name, f) bool AsyncCommand##name::execute(AudioPlayerState &state){ \
		return state.execute_##f(); \
	}

IMPLEMENT_ASYNC_COMMAND(HardPlay, hardplay)
IMPLEMENT_ASYNC_COMMAND(PlayPause, playpause)
IMPLEMENT_ASYNC_COMMAND(Play, play)
IMPLEMENT_ASYNC_COMMAND(Pause, pause)
IMPLEMENT_ASYNC_COMMAND(Stop, stop)
IMPLEMENT_ASYNC_COMMAND(Previous, previous)
IMPLEMENT_ASYNC_COMMAND(Next, next)
IMPLEMENT_ASYNC_COMMAND(Exit, exit)
IMPLEMENT_ASYNC_COMMAND(PlaybackEnd, playback_end)

bool AsyncCommandAbsoluteSeek::execute(AudioPlayerState &state){
	return state.execute_absolute_seek(this->param, this->scaling);
}

bool AsyncCommandRelativeSeek::execute(AudioPlayerState &state){
	return state.execute_relative_seek(this->seconds);
}

bool AsyncCommandLoad::execute(AudioPlayerState &state){
	return state.execute_load(this->load, this->file, this->path);
}
