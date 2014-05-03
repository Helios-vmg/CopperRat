#include "ResamplingFilter.h"
#include "CommonFunctions.h"
#include <cmath>
#include <cassert>

class AudioFilter{
protected:
	AudioFormat src_format;
	AudioFormat dst_format;
public:
	AudioFilter(const AudioFormat &src_format, const AudioFormat &dst_format): src_format(src_format), dst_format(dst_format){}
	virtual ~AudioFilter(){}
	virtual void read(audio_buffer_t &buffer) = 0;
};

class BitShiftingFilter : public AudioFilter{
public:
	BitShiftingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	static BitShiftingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
};

class ChannelMixingFilter : public AudioFilter{
public:
	ChannelMixingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	static ChannelMixingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
};

class ResamplingFilter : public AudioFilter{
public:
	ResamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	virtual ~ResamplingFilter(){}
	static ResamplingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
};

class UpsamplingFilter : public ResamplingFilter{
public:
	UpsamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): ResamplingFilter(src_format, dst_format){}
	virtual ~UpsamplingFilter(){}
	static UpsamplingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
};

template <typename NumberT, unsigned Channels>
class UpsamplingFilterGeneric : public UpsamplingFilter{
public:
	UpsamplingFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format): UpsamplingFilter(src_format, dst_format){}
	void read(audio_buffer_t &buffer){
		//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
		const unsigned src_rate = this->src_format.freq;
		const unsigned dst_rate = this->dst_format.freq;
		memory_sample_count_t samples_to_process = buffer.samples() * dst_rate / src_rate;
		memory_sample_count_t samples_unwritten = samples_to_process;
		const memory_audio_position_t original_last_sample = buffer.samples() - 1;
		const memory_sample_count_t n0 = samples_to_process - 1;
		const unsigned channel_count = this->dst_format.channels;
		while (--samples_unwritten){
			sample_t<NumberT, Channels> *dst_sample = buffer.get_sample<NumberT, Channels>(samples_unwritten);
			memory_audio_position_t src_sample_pos = samples_unwritten * src_rate / dst_rate;
			const sample_t<NumberT, Channels> *src_sample = buffer.get_sample<NumberT, Channels>(src_sample_pos);
			*dst_sample = *src_sample;
		}
		buffer.set_sample_count(samples_to_process);
	}
};

template <typename NumberT>
class UpsamplingFilterGeneric<NumberT, 0> : public UpsamplingFilter{
public:
	UpsamplingFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format): UpsamplingFilter(src_format, dst_format){}
	void read(audio_buffer_t &buffer){
		//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
		const unsigned src_rate = this->src_format.freq;
		const unsigned dst_rate = this->dst_format.freq;
		memory_sample_count_t samples_to_process = buffer.samples() * dst_rate / src_rate;
		memory_sample_count_t samples_unwritten = samples_to_process;
		const memory_audio_position_t original_last_sample = buffer.samples() - 1;
		const memory_sample_count_t n0 = samples_to_process - 1;
		const unsigned channel_count = this->dst_format.channels;
		while (--samples_unwritten){
			sample_t<NumberT, 1> *dst_sample = buffer.get_sample_use_channels<NumberT>(samples_unwritten);
			memory_audio_position_t src_sample_pos = samples_unwritten * src_rate / dst_rate;
			const sample_t<NumberT, 1> *src_sample = buffer.get_sample_use_channels<NumberT>(src_sample_pos);
			memcpy(dst_sample, src_sample, sizeof(NumberT) * channel_count);
		}
		buffer.set_sample_count(samples_to_process);
	}
};

template <typename NumberT, unsigned Channels>
class UpsamplingFilterPower : public UpsamplingFilter{
	unsigned power;
public:
	UpsamplingFilterPower(const AudioFormat &src_format, const AudioFormat &dst_format, unsigned power): UpsamplingFilter(src_format, dst_format), power(power){}
	void read(audio_buffer_t &buffer){
		//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
		const unsigned src_rate = this->src_format.freq;
		const unsigned dst_rate = this->dst_format.freq;
		memory_sample_count_t samples_to_process = buffer.samples() * dst_rate / src_rate;
		memory_sample_count_t samples_unwritten = samples_to_process;
		const memory_audio_position_t original_last_sample = buffer.samples() - 1;
		const memory_sample_count_t n0 = samples_to_process - 1;
		const unsigned channel_count = this->dst_format.channels;
		unsigned times = 1 << this->power;
		sample_t<NumberT, Channels> *dst_sample = buffer.get_sample<NumberT, Channels>(samples_unwritten);
		while (samples_unwritten){
			memory_audio_position_t src_sample_pos = (samples_unwritten - 1) >> this->power;
			sample_t<NumberT, Channels> src_sample = *buffer.get_sample<NumberT, Channels>(src_sample_pos);
			for (unsigned i = 0; i != times && --samples_unwritten; i++)
				*(dst_sample--) = src_sample;
		}
		buffer.set_sample_count(samples_to_process);
	}
};

template <typename NumberT>
class UpsamplingFilterPower<NumberT, 0> : public UpsamplingFilter{
	unsigned power;
public:
	UpsamplingFilterPower(const AudioFormat &src_format, const AudioFormat &dst_format, unsigned power): UpsamplingFilter(src_format, dst_format), power(power){}
	void read(audio_buffer_t &buffer){
		//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
		const unsigned src_rate = this->src_format.freq;
		const unsigned dst_rate = this->dst_format.freq;
		memory_sample_count_t samples_to_process = buffer.samples() * dst_rate / src_rate;
		memory_sample_count_t samples_unwritten = samples_to_process;
		const memory_audio_position_t original_last_sample = buffer.samples() - 1;
		const memory_sample_count_t n0 = samples_to_process - 1;
		const unsigned channel_count = this->dst_format.channels;
		unsigned times = 1 << this->power;
		sample_t<NumberT, 1> *dst_sample = buffer.get_sample_use_channels<NumberT>(samples_unwritten);
		while (samples_unwritten){
			memory_audio_position_t src_sample_pos = (samples_unwritten - 1) >> this->power;
			sample_t<NumberT, 1> *src_sample = buffer.get_sample_use_channels<NumberT>(src_sample_pos);
			for (unsigned i = 0; i != times && --samples_unwritten; i++){
				memcpy(dst_sample, src_sample, sizeof(NumberT) * channel_count);
				dst_sample -= channel_count;
			}
		}
		buffer.set_sample_count(samples_to_process);
	}
};

class DownsamplingFilter : public ResamplingFilter{
public:
	DownsamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): ResamplingFilter(src_format, dst_format){}
	void read(audio_buffer_t &buffer);
};

AudioFilterManager::AudioFilterManager(Decoder &decoder, const AudioFormat &dst_format): decoder(decoder){
	AudioFormat src_format = decoder.get_audio_format();
#if 0
	//TODO
	if (src_format.bytes_per_channel != dst_format.bytes_per_channel)
		this->filters.push_back(BitShiftingFilter::create(src_format, dst_format));
	if (src_format.channels != dst_format.channels)
		this->filters.push_back(ChannelMixingFilter::create(src_format, dst_format));
#endif
	if (src_format.freq != dst_format.freq)
		this->filters.push_back(ResamplingFilter::create(src_format, dst_format));
}

AudioFilterManager::~AudioFilterManager(){
	for (size_t i = 0; i < this->filters.size(); i++)
		delete this->filters[i];
}

audio_buffer_t AudioFilterManager::read(audio_position_t position, size_t &samples_read_from_decoder){
	audio_buffer_t buffer = this->decoder.read_more(position);
	if (!buffer)
		return buffer;
	samples_read_from_decoder = buffer.samples();
	audio_buffer_t ret = buffer.clone_with_minimum_length(buffer.byte_length() * 4);
	for (size_t i = 0; i < this->filters.size(); i++)
		this->filters[i]->read(ret);
	return ret;
}

ResamplingFilter *ResamplingFilter::create(const AudioFormat &src_format, const AudioFormat &dst_format){
	if (src_format.freq < dst_format.freq)
		return UpsamplingFilter::create(src_format, dst_format);
	return new DownsamplingFilter(src_format, dst_format);
}

template <unsigned N>
inline UpsamplingFilter *UpsamplingFilter_Creator_helper(const AudioFormat &src_format, const AudioFormat &dst_format){
	unsigned src_rate = src_format.freq;
	unsigned dst_rate = dst_format.freq;
	unsigned div = gcd(src_rate, dst_rate);
	unsigned dividend = dst_rate / div;
	if (div == src_rate && is_power_of_2(dividend)){
		unsigned log = integer_log2(dividend);
		return new UpsamplingFilterPower<Sint16, N>(src_format, dst_format, log);
	}
	return new UpsamplingFilterGeneric<Sint16, N>(src_format, dst_format);
}

template <unsigned I, unsigned N>
struct UpsamplingFilter_Creator{
	static UpsamplingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format,unsigned channels){
		if (channels == I)
			return UpsamplingFilter_Creator_helper<I>(src_format, dst_format);
		return UpsamplingFilter_Creator<I + 1, N>::create(src_format, dst_format, channels);
	}
};

template <unsigned N>
struct UpsamplingFilter_Creator<N, N>{
	static UpsamplingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format, unsigned channels){
		return UpsamplingFilter_Creator_helper<0>(src_format, dst_format);
	}
};

UpsamplingFilter *UpsamplingFilter::create(const AudioFormat &src_format, const AudioFormat &dst_format){
	return UpsamplingFilter_Creator<1, 2>::create(src_format, dst_format, dst_format.channels);
}

void DownsamplingFilter::read(audio_buffer_t &buffer){
	sample_count_t samples_to_process = buffer.samples();
	memory_sample_count_t samples_written = 0;
	const unsigned src_rate = this->src_format.freq;
	const unsigned dst_rate = this->dst_format.freq;
	sample_t<Sint16, 1> *dst_sample = buffer.get_sample_use_channels<Sint16>(0);
	const unsigned channel_count = this->dst_format.channels;
	double fraction = double(src_rate) / double(dst_rate);
	for (; samples_written != samples_to_process; samples_written++){
		double src_sample0 = samples_written * fraction;
		double src_sample1 = (samples_written + 1) * fraction;
		double accumulators[256] = {0};
		double count=0;
		bool end = 0;
		for (double j = src_sample0; j < src_sample1;){
			double next = floor(j + 1);
			const sample_t<Sint16, 1> *sample = buffer.get_sample_use_channels<Sint16>((memory_audio_position_t)j);
			double limit = (next < src_sample1) ? next : src_sample1;
			for (unsigned channel = 0; channel < channel_count; channel++){
				double value = s16_to_double(sample->values[channel]);
				accumulators[channel] += (limit - j) * value;
			}
			count += limit - j;
			j = next;
		}
		assert(count);
		if (count){
			for (unsigned channel = 0; channel < channel_count; channel++)
				dst_sample->values[0] = double_to_s16(accumulators[channel] / count);
		}
	}
	buffer.set_sample_count(samples_written);
}
