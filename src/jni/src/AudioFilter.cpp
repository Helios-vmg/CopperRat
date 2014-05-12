#include "AudioFilter.h"
#include "AudioFilterPrivate.h"
#include "CommonFunctions.h"
#include <cmath>
#include <cassert>

AudioFilterManager::AudioFilterManager(Decoder &decoder, const AudioFormat &dst_format): decoder(decoder), dst_format(dst_format), filter_allocated(0), need_two_buffers(1){
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

audio_buffer_t AudioFilterManager::read(memory_sample_count_t &samples_read_from_decoder){
	audio_buffer_t buffers[2];
	unsigned buffers_size = 0;
	if (!this->need_two_buffers && this->saved_buffer)
		buffers[buffers_size++] = this->saved_buffer;
	this->need_two_buffers = 0;
	while (buffers_size < 2){
		audio_buffer_t buffer = this->decoder.read();
		if (!buffer)
			break;
		buffers[buffers_size++] = buffer;
	}
	samples_read_from_decoder = !buffers_size ? 0 : buffers[0].samples();
	if (!this->filter_allocated && this->decoder.lazy_filter_allocation())
		this->allocate_filters();
	if (buffers_size > 1)
		this->saved_buffer = buffers[1];
	else
		this->saved_buffer.unref();
	if (buffers_size || !buffers[0] || this->dont_convert)
		return buffers[0];
	size_t bytes_required = buffers[0].byte_length(),
		max_bytes = bytes_required;
	for (size_t i = 0; i < this->filters.size(); i++){
		if (!this->filters[i])
			continue;
		bytes_required = this->filters[i]->calculate_required_byte_size(bytes_required);
		max_bytes = std::max(max_bytes, bytes_required);
	}
	buffers[0] = buffers[0].clone_with_minimum_byte_length(max_bytes, &this->dst_format);
	for (size_t i = 0; i < this->filters.size(); i++)
		this->filters[i]->read(buffers, buffers_size);
	return buffers[0];
}
