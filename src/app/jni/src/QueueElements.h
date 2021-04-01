#pragma once

#include "AudioBuffer.h"
#include "Exception.h"

class AudioPlayer;
class AudioPlayerState;
class UserInterface;
class GenericMetadata;

#define AudioCallback_switch_SIGNATURE2(x)              \
	bool x AudioCallback_switch(                        \
		AudioPlayer *player,                            \
		AudioPlayerState *&state,                       \
		Uint8 *stream,                                  \
		int len,                                        \
		unsigned bytes_per_sample,                      \
		memory_sample_count_t &samples_written,         \
		audio_position_t &last_position,                \
		unsigned &sample_rate,                          \
		std::shared_ptr<InternalQueueElement> pointer   \
	)

#define NULL_MACRO
#define AudioCallback_switch_SIGNATURE AudioCallback_switch_SIGNATURE2(NULL_MACRO)

class InternalQueueElement{
protected:
	AudioPlayerState *state;
public:
	InternalQueueElement(AudioPlayerState *state = nullptr): state(state){}
	virtual ~InternalQueueElement(){}
	virtual AudioCallback_switch_SIGNATURE = 0;
	virtual bool is_buffer() const = 0;
};

class BufferQueueElement : public InternalQueueElement{
	audio_buffer_t buffer;
	AudioFormat stream_format;
public:
	BufferQueueElement(AudioPlayerState *state, audio_buffer_t buffer, const AudioFormat &stream_format):
		InternalQueueElement(state),
		buffer(buffer),
		stream_format(stream_format){}
	audio_buffer_t get_buffer(){
		return this->buffer;
	}
	AudioCallback_switch_SIGNATURE;
	bool is_buffer() const{
		return true;
	}
};

class PlaybackEnd : public InternalQueueElement{
public:
	AudioCallback_switch_SIGNATURE;
	bool is_buffer() const{
		return false;
	}
};

class ExternalQueueElement : public InternalQueueElement{
public:
	ExternalQueueElement(AudioPlayerState *state = nullptr): InternalQueueElement(state){}
	virtual ~ExternalQueueElement(){}
	void push(AudioPlayer *player, std::shared_ptr<InternalQueueElement> pointer);
	virtual AudioCallback_switch_SIGNATURE{
		state = this->state;
		this->push(player, pointer);
		return true;
	}
	bool is_buffer() const{
		return false;
	}
	virtual unsigned receive(UserInterface &) = 0;
};

class ExceptionTransport : public ExternalQueueElement{
	CR_Exception e;
public:
	ExceptionTransport(const CR_Exception &e): ExternalQueueElement(), e(e){}
	unsigned receive(UserInterface &ui){
		throw this->e;
		return false;
	}
};

class TotalTimeUpdate : public ExternalQueueElement{
	double seconds;
public:
	TotalTimeUpdate(AudioPlayerState *state, double seconds): ExternalQueueElement(state), seconds(seconds){}
	double get_seconds(){
		return this->seconds;
	}
	unsigned receive(UserInterface &ui);
};

class MetaDataUpdate : public ExternalQueueElement{
	std::shared_ptr<GenericMetadata> metadata;
public:
	MetaDataUpdate(AudioPlayerState *state, std::shared_ptr<GenericMetadata> metadata): ExternalQueueElement(state), metadata(metadata){}
	std::shared_ptr<GenericMetadata> get_metadata(){
		return this->metadata;
	}
	unsigned receive(UserInterface &ui);
};

class PlaybackStop : public ExternalQueueElement{
public:
	unsigned receive(UserInterface &ui);
};

class ExitAcknowledged : public ExternalQueueElement{
public:
	unsigned receive(UserInterface &ui){
		return 0;
	}
};
