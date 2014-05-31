#ifndef DECODER_H
#define DECODER_H
#include "AudioTypes.h"
#include "Metadata.h"
#include "AudioBuffer.h"
#include <limits>

class AudioStream;

class Decoder{
	audio_position_t current_position;
	audio_position_t length;
	double seconds_length;
protected:
	AudioStream &parent;
	std::wstring path;
	//OggMetadata metadata;
	virtual audio_buffer_t read_more_internal() = 0;
	virtual sample_count_t get_pcm_length_internal() = 0;
	virtual double get_seconds_length_internal() = 0;
public:
	Decoder(AudioStream &parent, const std::wstring &path):
		parent(parent),
		//metadata(path),
		path(path),
		current_position(0),
		length(invalid_sample_count),
		seconds_length(-1){}
	virtual ~Decoder(){}
	virtual AudioFormat get_audio_format() = 0;
	virtual bool lazy_filter_allocation(){
		return 0;
	}
	audio_buffer_t read();
	static Decoder *create(AudioStream &, const std::wstring &path);
	virtual bool seek(audio_position_t) = 0;
	virtual bool fast_seek(audio_position_t p, audio_position_t &new_position){
		bool ret = this->seek(p);
		if (ret)
			new_position = p;
		return ret;
	}
	virtual bool fast_seek_seconds(double p, audio_position_t &new_position){
		return 0;
	}
	sample_count_t get_pcm_length(){
		if (this->length != invalid_sample_count)
			return this->length;
		return this->length = this->get_pcm_length_internal();
	}
	double get_seconds_length(){
		if (this->seconds_length >= 0)
			return this->seconds_length;
		return this->seconds_length = this->get_seconds_length_internal();
	}
	virtual bool fast_seek_takes_seconds() const{
		return 0;
	}
};

class DecoderException{};
class DecoderInitializationException : public DecoderException{};

#endif
