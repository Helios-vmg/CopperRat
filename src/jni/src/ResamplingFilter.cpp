#include "ResamplingFilter.h"
#include "CommonFunctions.h"
#include <cmath>

ResamplingFilter *ResamplingFilter::create(Decoder &decoder, unsigned d){
	unsigned src_rate = decoder.get_sampling_rate();
	if (src_rate == d)
		return new ResamplingFilter(decoder, d);
	if (src_rate < d)
		return new UpsamplingFilter(decoder, d);
	return new DownsamplingFilter(decoder, d);
}


sample_count_t ResamplingFilter::read(audio_buffer_t buffer, audio_position_t position){
	return this->decoder.direct_output(buffer, position);
}

sample_count_t UpsamplingFilter::read(audio_buffer_t buffer, audio_position_t position){
	//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
	sample_count_t samples_read = 0;
	for (unsigned i = 0; i < buffer.sample_count; i++){
		//This multiplication:
		//    position*this->src_rate
		//would need, pessimistically, a position value greater than 15 years in order to overflow.
		//(2^64-1)/192000/192000/86400/365.2425 ~= 15.86
		audio_position_t src_sample = (position + i) * this->src_rate / this->dst_rate;
		const sample_t *sample = this->decoder[src_sample];
		if (!sample)
			break;
		for (unsigned j = 0; j < buffer.channel_count; j++)
			*(buffer.data++) = *(sample++);
		samples_read++;
	}
	return samples_read;
}

sample_count_t DownsamplingFilter::read(audio_buffer_t buffer, audio_position_t position){
	sample_count_t samples_read = 0;
	for (unsigned i = 0; i < buffer.sample_count; i++){
		double fraction = double(this->src_rate) / double(this->dst_rate);
		double src_sample0 = position*fraction;
		double src_sample1 = (position + 1) * fraction;
		double accumulators[256] = {0};
		double count=0;
		bool end=0;
		for (double j = src_sample0; j < src_sample1;){
			double next = floor(j + 1);
			const sample_t *sample = this->decoder[(audio_position_t)j];
			if (!sample){
				end = 1;
				break;
			}
			double limit = (next < src_sample1) ? next : src_sample1;
			for (unsigned channel = 0; channel < buffer.channel_count; channel++){
				double value = s16_to_double(sample[channel]);
				accumulators[channel] += (limit - j) * value;
			}
			count += limit - j;
			j = next;
		}
		if (count){
			for (unsigned channel = 0; channel < buffer.channel_count; channel++)
				*(buffer.data++) = double_to_s16(accumulators[channel] / count);
			samples_read++;
		}
		if (end)
			break;
	}
	return samples_read;
}
