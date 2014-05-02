#ifndef RESAMPLING_FILTER_H
#define RESAMPLING_FILTER_H
#include "Decoder.h"

class ResamplingFilter{
protected:
	Decoder &decoder;
	unsigned src_rate;
	unsigned dst_rate;
public:
	ResamplingFilter(Decoder &decoder, unsigned dst_rate);
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
	virtual ~UpsamplingFilter(){}
	static UpsamplingFilter *create(Decoder &decoder, unsigned dst_rate);
};

class UpsamplingFilterGeneric : public UpsamplingFilter{
public:
	UpsamplingFilterGeneric(Decoder &decoder, unsigned dst_rate): UpsamplingFilter(decoder, dst_rate){}
	sample_count_t read(audio_buffer_t buffer, audio_position_t position);
};

class UpsamplingFilterPower : public UpsamplingFilter{
	unsigned power;
public:
	UpsamplingFilterPower(Decoder &decoder, unsigned dst_rate, unsigned power): UpsamplingFilter(decoder, dst_rate), power(power){}
	sample_count_t read(audio_buffer_t buffer, audio_position_t position);
};

class UpsamplingFilterPowerStereo : public UpsamplingFilter{
	unsigned power;
public:
	UpsamplingFilterPowerStereo(Decoder &decoder, unsigned dst_rate, unsigned power): UpsamplingFilter(decoder, dst_rate), power(power){}
	sample_count_t read(audio_buffer_t buffer, audio_position_t position);
};

class DownsamplingFilter : public ResamplingFilter{
public:
	DownsamplingFilter(Decoder &decoder, unsigned dst_rate): ResamplingFilter(decoder, dst_rate){}
	sample_count_t read(audio_buffer_t buffer, audio_position_t position);
};

#endif
