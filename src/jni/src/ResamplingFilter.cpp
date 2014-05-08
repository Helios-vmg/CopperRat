#include "AudioFilterPrivate.h"
#include "CommonFunctions.h"
#include <cassert>

class UpsamplingFilter : public ResamplingFilter{
public:
	UpsamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): ResamplingFilter(src_format, dst_format){}
	virtual ~UpsamplingFilter(){}
	static UpsamplingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
};

class DownsamplingFilter : public ResamplingFilter{
public:
	DownsamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): ResamplingFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size);
};

template <typename NumberT, unsigned Channels, bool PowerVersion>
struct UpsamplingFilterCondTypes{
	typedef sample_t<NumberT, Channels> *dst_sample_t;
	typedef const sample_t<NumberT, Channels> *src_sample_t;
};

template <typename NumberT, bool PowerVersion>
struct UpsamplingFilterCondTypes<NumberT, 0, PowerVersion>{
	typedef sample_t<NumberT, 1> *dst_sample_t;
	typedef const sample_t<NumberT, 1> *src_sample_t;
};

template <typename NumberT, unsigned Channels, bool PowerVersion>
struct UpsamplingFilterCond{
	const unsigned power,
		times,
		src_rate,
		dst_rate;
	audio_buffer_t buffer;
	memory_sample_count_t samples_unwritten;
	const unsigned channel_count;
	typedef typename UpsamplingFilterCondTypes<NumberT, Channels, PowerVersion>::dst_sample_t dst_sample_t;
	typedef typename UpsamplingFilterCondTypes<NumberT, Channels, PowerVersion>::src_sample_t src_sample_t;
	UpsamplingFilterCond(
			audio_buffer_t buffer,
			unsigned power,
			memory_sample_count_t samples_unwritten,
			unsigned channel_count,
			unsigned src_rate,
			unsigned dst_rate):
		power(power),
		times(1 << power),
		buffer(buffer),
		samples_unwritten(samples_unwritten),
		channel_count(channel_count),
		src_rate(src_rate),
		dst_rate(dst_rate){}
	bool continue_loop(){
		if (!PowerVersion)
			return !!--this->samples_unwritten;
		return !!this->samples_unwritten;
	}
	dst_sample_t get_dst(){
		if (Channels)
			return (dst_sample_t)buffer.get_sample<NumberT, Channels>(this->samples_unwritten);
		return (dst_sample_t)buffer.get_sample_use_channels<NumberT>(this->samples_unwritten);
	}
	src_sample_t get_src(){
		memory_audio_position_t src_sample_pos;
		if (PowerVersion)
			src_sample_pos = (samples_unwritten - 1) >> this->power;
		else
			src_sample_pos = samples_unwritten * src_rate / dst_rate;
		src_sample_t ret;
		if (Channels)
			ret = (src_sample_t)buffer.get_sample<NumberT, Channels>(src_sample_pos);
		else
			ret = (src_sample_t)buffer.get_sample_use_channels<NumberT>(src_sample_pos);
		return ret;
	}
	void do_assignment(dst_sample_t dst, src_sample_t src){
		if (!PowerVersion){
			if (Channels)
				*dst = *src;
			else
				memcpy(dst, src, sizeof(NumberT) * this->channel_count);
		}else{
			if (Channels){
				UNPOINTER(src_sample_t) src_sample = *src;
				for (unsigned i = 0; i != this->times && --this->samples_unwritten; i++)
					*(dst--) = src_sample;
			}else{
				for (unsigned i = 0; i != this->times && --this->samples_unwritten; i++){
					memcpy(dst, src, sizeof(NumberT) * this->channel_count);
					dst -= this->channel_count;
				}
			}
		}
	}
};

template <typename NumberT, unsigned Channels, bool PowerVersion>
class UpsamplingFilterGeneric : public UpsamplingFilter{
	unsigned power;
public:
	UpsamplingFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format, unsigned power = 0): UpsamplingFilter(src_format, dst_format), power(power){}
	//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
	void read(audio_buffer_t *buffers, size_t size){
		audio_buffer_t &buffer = buffers[0];
		typedef typename UpsamplingFilterCond<NumberT, Channels, PowerVersion>::dst_sample_t dst_sample_t;
		typedef typename UpsamplingFilterCond<NumberT, Channels, PowerVersion>::src_sample_t src_sample_t;
		unsigned dst_rate = this->dst_format.freq;
		unsigned src_rate = this->src_format.freq;
		memory_sample_count_t samples_to_process = buffer.samples() * dst_rate / src_rate;
		UpsamplingFilterCond<NumberT, Channels, PowerVersion> ufc(
			buffer,
			power,
			samples_to_process,
			this->dst_format.channels,
			src_rate,
			dst_rate
		);
		while (ufc.continue_loop())
			ufc.do_assignment(ufc.get_dst(), ufc.get_src());
		buffer.set_sample_count(samples_to_process);
	}
};

template <typename NumberT, unsigned Channels>
struct UpsamplingFilterCondFloat{
	const unsigned src_rate,
		dst_rate;
	float fraction;
	bool no_interpolation;
	audio_buffer_t *buffers;
	unsigned buffers_size;
	memory_sample_count_t samples_in_src;
	memory_sample_count_t samples_unwritten;
	const unsigned channel_count;
	typedef typename UpsamplingFilterCondTypes<NumberT, Channels, true>::dst_sample_t dst_sample_t;
	typedef typename UpsamplingFilterCondTypes<NumberT, Channels, true>::src_sample_t src_sample_t;
	UpsamplingFilterCondFloat(
			audio_buffer_t *buffers,
			unsigned buffers_size,
			memory_sample_count_t samples_in_src,
			memory_sample_count_t samples_unwritten,
			unsigned channel_count,
			unsigned src_rate,
			unsigned dst_rate):
		buffers(buffers),
		buffers_size(buffers_size),
		samples_in_src(samples_in_src),
		samples_unwritten(samples_unwritten),
		channel_count(channel_count),
		src_rate(src_rate),
		dst_rate(dst_rate),
		fraction(float(src_rate) / float(dst_rate)){}
	bool continue_loop(){
		return !!--this->samples_unwritten;
	}
	dst_sample_t get_dst(){
		if (Channels)
			return (dst_sample_t)buffers[0].get_sample<NumberT, Channels>(this->samples_unwritten);
		return (dst_sample_t)buffers[0].get_sample_use_channels<NumberT>(this->samples_unwritten);
	}
	src_sample_t get_src0() const{
		memory_audio_position_t src_sample_pos = samples_unwritten * src_rate / dst_rate;
		src_sample_t ret;
		if (Channels)
			ret = (src_sample_t)buffers[0].get_sample<NumberT, Channels>(src_sample_pos);
		else
			ret = (src_sample_t)buffers[0].get_sample_use_channels<NumberT>(src_sample_pos);
		return ret;
	}
	src_sample_t get_src1() const{
		memory_audio_position_t src_sample_pos = samples_unwritten * src_rate / dst_rate + 1;
		unsigned idx = src_sample_pos >= this->samples_in_src;
		if (idx){
			if (buffers_size < 2)
				return get_src0();
			src_sample_pos = 0;
		}
		src_sample_t ret;
		if (Channels)
			ret = (src_sample_t)buffers[idx].get_sample<NumberT, Channels>(src_sample_pos);
		else
			ret = (src_sample_t)buffers[idx].get_sample_use_channels<NumberT>(src_sample_pos);
		return ret;
	}
	void do_assignment(dst_sample_t dst, src_sample_t src0, src_sample_t src1){
		UNPOINTER(dst_sample_t) temp;
		float fractional_part = this->samples_unwritten * this->fraction;
		fractional_part -= floor(fractional_part);
		for (unsigned channel = 0; channel < Channels; channel++){
			temp.values[channel] = src0->values[channel] * (1.0 - fractional_part) + src1->values[channel] * fractional_part;
		}
		if (Channels)
			*dst = temp;
		else
			memcpy(dst, &temp, sizeof(temp));
	}
};

#include <iostream>

template <typename NumberT, unsigned Channels>
class UpsamplingFilterFloat : public UpsamplingFilter{
public:
	UpsamplingFilterFloat(const AudioFormat &src_format, const AudioFormat &dst_format): UpsamplingFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){
		audio_buffer_t &buffer = buffers[0];
		typedef typename UpsamplingFilterCondFloat<NumberT, Channels>::src_sample_t src_sample_t;
		unsigned dst_rate = this->dst_format.freq;
		unsigned src_rate = this->src_format.freq;
		memory_sample_count_t samples_to_process = buffer.samples() * dst_rate / src_rate;
		static bool printed = 0;
		if (!printed){
			std::cout <<samples_to_process<<std::endl;
			printed = 1;
		}
		UpsamplingFilterCondFloat<NumberT, Channels> ufc(
			buffers,
			size,
			buffers[0].samples(),
			samples_to_process,
			this->dst_format.channels,
			src_rate,
			dst_rate
		);
		while (ufc.continue_loop())
			ufc.do_assignment(ufc.get_dst(), ufc.get_src0(), ufc.get_src1());
		buffer.set_sample_count(samples_to_process);
	}
};

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
	//return new UpsamplingFilterFloat<Sint16, N>(src_format, dst_format);
	if (div == src_rate && is_power_of_2(dividend)){
		unsigned log = integer_log2(dividend);
		return new UpsamplingFilterGeneric<Sint16, N, true>(src_format, dst_format, log);
	}
	return new UpsamplingFilterGeneric<Sint16, N, false>(src_format, dst_format);
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
	return UpsamplingFilter_Creator<1, 3>::create(src_format, dst_format, dst_format.channels);
}

void DownsamplingFilter::read(audio_buffer_t *buffers, size_t size){
	audio_buffer_t &buffer = buffers[0];
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
