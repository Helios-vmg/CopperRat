#include "ResamplingFilter.h"
#include "CommonFunctions.h"
#include <cmath>

ResamplingFilter::ResamplingFilter(Decoder &decoder, unsigned dst_rate): decoder(decoder), src_rate(decoder.get_sampling_rate()), dst_rate(dst_rate){
}

ResamplingFilter *ResamplingFilter::create(Decoder &decoder, unsigned d){
	unsigned src_rate = decoder.get_sampling_rate();
	if (src_rate == d)
		return new IdentityResamplingFilter(decoder, d);
	if (src_rate < d)
		return UpsamplingFilter::create(decoder, d);
	return new DownsamplingFilter(decoder, d);
}

sample_count_t IdentityResamplingFilter::read(audio_buffer_t buffer, audio_position_t position){
	return this->decoder.direct_output(buffer, position);
}

UpsamplingFilter *UpsamplingFilter::create(Decoder &decoder, unsigned dst_rate){
	unsigned src_rate = decoder.get_sampling_rate();
	unsigned div = gcd(src_rate, dst_rate);
	unsigned dividend = dst_rate / div;
	if (div == src_rate && is_power_of_2(dividend)){
		if (decoder.get_channel_count() == 2)
			return new UpsamplingFilterPowerStereo(decoder, dst_rate, integer_log2(dividend));
		return new UpsamplingFilterPower(decoder, dst_rate, integer_log2(dividend));
	}
	return new UpsamplingFilterGeneric(decoder, dst_rate);
}

sample_count_t UpsamplingFilterPower::read(audio_buffer_t buffer, audio_position_t position){
	//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
	sample_count_t samples_read = 0;
	unsigned times = 1 << this->power;
	for (; samples_read != buffer.sample_count; samples_read++){
		audio_position_t src_sample = (position + samples_read) >> this->power;
		const sample_t *sample = this->decoder[src_sample];
		if (!sample)
			break;
		for (unsigned i = 0; i != times && samples_read != buffer.sample_count; samples_read++, i++){
			const sample_t *temp = sample;
			switch (buffer.channel_count){
				case 7:
					*(buffer.data++) = *(temp++);
				case 6:
					*(buffer.data++) = *(temp++);
				case 5:
					*(buffer.data++) = *(temp++);
				case 4:
					*(buffer.data++) = *(temp++);
				case 3:
					*(buffer.data++) = *(temp++);
				case 2:
					*(buffer.data++) = *(temp++);
				case 1:
					*(buffer.data++) = *temp;
			}
		}
	}
	return samples_read;
}

sample_count_t UpsamplingFilterPowerStereo::read(audio_buffer_t buffer, audio_position_t position){
	//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
	sample_count_t samples_read = 0;
	unsigned times = 1 << this->power;
	stereo_sample_t *dst = (stereo_sample_t *)buffer.data;
	const stereo_sample_t *sample_p;
	while (1){
		if (samples_read == buffer.sample_count)
			break;
		sample_p = (const stereo_sample_t *)this->decoder[(position + samples_read) >> this->power];
		if (!sample_p)
			break;
		stereo_sample_t sample = *sample_p;
		for (unsigned i = 0; i != times && samples_read != buffer.sample_count; samples_read++, i++)
			*(dst++) = sample;
	}
	return samples_read;
}

sample_count_t UpsamplingFilterGeneric::read(audio_buffer_t buffer, audio_position_t position){
	//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
	sample_count_t samples_read = 0;
	for (; samples_read != buffer.sample_count; samples_read++){
		audio_position_t src_sample = (position + samples_read) * this->src_rate / this->dst_rate;
		const sample_t *sample = this->decoder[src_sample];
		if (!sample)
			break;
		switch (buffer.channel_count){
			case 7:
				*(buffer.data++) = *(sample++);
			case 6:
				*(buffer.data++) = *(sample++);
			case 5:
				*(buffer.data++) = *(sample++);
			case 4:
				*(buffer.data++) = *(sample++);
			case 3:
				*(buffer.data++) = *(sample++);
			case 2:
				*(buffer.data++) = *(sample++);
			case 1:
				*(buffer.data++) = *(sample++);
		}
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
