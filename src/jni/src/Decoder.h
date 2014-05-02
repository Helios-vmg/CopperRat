#ifndef DECODER_H
#define DECODER_H
#include "Queue.h"
#include "AudioTypes.h"

class Decoder{
	audio_position_t current_position;
protected:
	bool offset_operator_helper(const sample_t *&ret, audio_position_t sample, int index);
	bool direct_output_helper(sample_count_t &ret, audio_buffer_t &buffer, audio_position_t &initial_position, int index);
	virtual bool seek(audio_position_t) = 0;
	virtual audio_buffer_t read_more() = 0;
public:
	Decoder(): current_position(0){}
	virtual ~Decoder(){}
	virtual AudioFormat get_audio_format() = 0;
	virtual unsigned get_sampling_rate() = 0;
	virtual unsigned get_channel_count() = 0;
	audio_buffer_t read_more(audio_position_t initial_position);
	static Decoder *create(const char *);
};

#endif
