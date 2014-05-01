#ifndef RESAMPLING_FILTER_H
#define RESAMPLING_FILTER_H
#include "Decoder.h"

class ResamplingFilter{
protected:
	Decoder &decoder;
	unsigned src_rate;
	unsigned dst_rate;
public:
	ResamplingFilter(Decoder &decoder, unsigned dst_rate): decoder(decoder), src_rate(decoder.get_sampling_rate()), dst_rate(dst_rate){}
	virtual ~ResamplingFilter(){}
	virtual sample_count_t read(audio_buffer_t buffer, audio_position_t position) = 0;
	static ResamplingFilter *create(Decoder &decoder, unsigned dst_rate);
};

class IdentityResamplingFilter : public ResamplingFilter{
public:
	IdentityResamplingFilter(Decoder &decoder, unsigned dst_rate): ResamplingFilter(decoder, dst_rate){}
	sample_count_t read(audio_buffer_t buffer, audio_position_t position);
};

class UpsamplingFilter : public ResamplingFilter{
public:
	UpsamplingFilter(Decoder &decoder, unsigned dst_rate): ResamplingFilter(decoder, dst_rate){}
	sample_count_t read(audio_buffer_t buffer, audio_position_t position);
};

class DownsamplingFilter : public ResamplingFilter{
public:
	DownsamplingFilter(Decoder &decoder, unsigned dst_rate): ResamplingFilter(decoder, dst_rate){}
	sample_count_t read(audio_buffer_t buffer, audio_position_t position);
};

#endif
