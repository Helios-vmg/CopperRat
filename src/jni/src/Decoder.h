#ifndef DECODER_H
#define DECODER_H
#include "Queue.h"

class Decoder{
protected:
	RingBufferQueue buffers;

	bool offset_operator_helper(const sample_t *&ret, audio_position_t sample, int index);
	bool direct_output_helper(sample_count_t &ret, audio_buffer_t &buffer, audio_position_t &initial_position, int index);
	virtual audio_buffer_t read_more() = 0;
	bool read_next();
public:
	virtual ~Decoder(){}
	virtual unsigned get_sampling_rate() = 0;
	const sample_t *operator[](audio_position_t);
	sample_count_t direct_output(audio_buffer_t buffer, audio_position_t initial_position);
	static Decoder *create(const char *);
};

#endif
