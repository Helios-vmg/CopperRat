#ifndef DECODER_H
#define DECODER_H
#include "Queue.h"
#include "AudioTypes.h"

class Decoder{
	audio_position_t current_position;
protected:
	virtual bool seek(audio_position_t) = 0;
	virtual audio_buffer_t read_more() = 0;
public:
	Decoder(): current_position(0){}
	virtual ~Decoder(){}
	virtual AudioFormat get_audio_format() = 0;
	virtual bool lazy_filter_allocation(){
		return 0;
	}
	audio_buffer_t read_more(audio_position_t initial_position);
	static Decoder *create(const char *);
};

class DecoderException{};
class DecoderInitializationException : public DecoderException{};

#endif
