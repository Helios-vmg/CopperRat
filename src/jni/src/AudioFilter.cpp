#include "AudioFilter.h"
#include "AudioFilterPrivate.h"
#include "CommonFunctions.h"
#include <cmath>
#include <cassert>

AudioFilterManager::AudioFilterManager(Decoder &decoder, const AudioFormat &dst_format): decoder(decoder), dst_format(dst_format), filter_allocated(0){
	if (!decoder.lazy_filter_allocation())
		this->allocate_filters();
}

void AudioFilterManager::allocate_filters(){
	AudioFormat src_format = decoder.get_audio_format();
	AudioFormat temp = src_format;
	temp.is_signed = dst_format.is_signed;
	if (src_format.is_signed != temp.is_signed)
		this->filters.push_back(SignednessFilter::create(src_format, temp));
	temp.bytes_per_channel = dst_format.bytes_per_channel;
	if (src_format.bytes_per_channel != temp.bytes_per_channel)
		this->filters.push_back(BitShiftingFilter::create(src_format, temp));
	temp.channels = dst_format.channels;
	if (src_format.channels != temp.channels)
		this->filters.push_back(ChannelMixingFilter::create(src_format, temp));
	temp.freq = dst_format.freq;
	if (src_format.freq != temp.freq)
		this->filters.push_back(ResamplingFilter::create(src_format, temp));
	this->dont_convert = !this->filters.size();
	this->filter_allocated = 1;
}

AudioFilterManager::~AudioFilterManager(){
	for (size_t i = 0; i < this->filters.size(); i++)
		delete this->filters[i];
}

audio_buffer_t AudioFilterManager::read(audio_position_t position, memory_sample_count_t &samples_read_from_decoder){
	audio_buffer_t buffer = this->decoder.read_more(position);
	samples_read_from_decoder = buffer.samples();
	if (!this->filter_allocated && this->decoder.lazy_filter_allocation())
		this->allocate_filters();
	if (!buffer || this->dont_convert)
		return buffer;
	size_t bytes_required = buffer.byte_length(),
		max_bytes = bytes_required;
	for (size_t i = 0; i < this->filters.size(); i++){
		if (!this->filters[i])
			continue;
		bytes_required = this->filters[i]->calculate_required_byte_size(bytes_required);
		max_bytes = std::max(max_bytes, bytes_required);
	}
	audio_buffer_t ret = buffer.clone_with_minimum_byte_length(max_bytes, &this->dst_format);
	for (size_t i = 0; i < this->filters.size(); i++)
		this->filters[i]->read(ret);
	return ret;
}
