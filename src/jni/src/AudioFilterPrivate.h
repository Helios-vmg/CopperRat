#include "AudioFilter.h"

class AudioFilter{
protected:
	AudioFormat src_format;
	AudioFormat dst_format;
public:
	AudioFilter(const AudioFormat &src_format, const AudioFormat &dst_format): src_format(src_format), dst_format(dst_format){}
	virtual ~AudioFilter(){}
	virtual void read(audio_buffer_t &buffer) = 0;
	virtual memory_sample_count_t calculate_required_size(memory_sample_count_t) = 0;
};

class SignednessFilter : public AudioFilter{
public:
	SignednessFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	static SignednessFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
	memory_sample_count_t calculate_required_size(memory_sample_count_t samples){
		return samples;
	}
};

class BitShiftingFilter : public AudioFilter{
public:
	BitShiftingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	static BitShiftingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
	memory_sample_count_t calculate_required_size(memory_sample_count_t samples){
		return samples * this->dst_format.bytes_per_channel / this->src_format.bytes_per_channel;
	}
};

class ChannelMixingFilter : public AudioFilter{
public:
	ChannelMixingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	static ChannelMixingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
	memory_sample_count_t calculate_required_size(memory_sample_count_t samples){
		return samples * this->dst_format.channels / this->src_format.channels;
	}
};

class ResamplingFilter : public AudioFilter{
public:
	ResamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	virtual ~ResamplingFilter(){}
	static ResamplingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
	memory_sample_count_t calculate_required_size(memory_sample_count_t samples){
		return samples * this->dst_format.freq / this->src_format.freq;
	}
};

class UpsamplingFilter : public ResamplingFilter{
public:
	UpsamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): ResamplingFilter(src_format, dst_format){}
	virtual ~UpsamplingFilter(){}
	static UpsamplingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
};

class DownsamplingFilter : public ResamplingFilter{
public:
	DownsamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): ResamplingFilter(src_format, dst_format){}
	void read(audio_buffer_t &buffer);
};
