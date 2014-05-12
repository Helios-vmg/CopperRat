#ifndef DECODER_H
#define DECODER_H
#include "Queue.h"
#include "AudioTypes.h"
#include <limits>

class Decoder{
	audio_position_t current_position;
	audio_position_t length;
	double seconds_length;
protected:
	virtual audio_buffer_t read_more_internal() = 0;
	virtual sample_count_t get_pcm_length_internal() = 0;
	virtual double get_seconds_length_internal() = 0;
public:
	Decoder(): current_position(0), length(invalid_sample_count), seconds_length(-1){}
	virtual ~Decoder(){}
	virtual AudioFormat get_audio_format() = 0;
	virtual bool lazy_filter_allocation(){
		return 0;
	}
	audio_buffer_t read_more();
	static Decoder *create(const char *);
	virtual bool seek(audio_position_t) = 0;
	virtual bool fast_seek(audio_position_t p){
		return this->seek(p);
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
};

class DecoderException{};
class DecoderInitializationException : public DecoderException{};

#endif
