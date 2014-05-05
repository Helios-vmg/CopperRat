#include "AudioFilter.h"
#include "AudioFilterPrivate.h"
#include "CommonFunctions.h"
#include <cmath>
#include <cassert>

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
	this->dont_convert = !this->filters.size();
}

AudioFilterManager::~AudioFilterManager(){
	for (size_t i = 0; i < this->filters.size(); i++)
		delete this->filters[i];
}

audio_buffer_t AudioFilterManager::read(audio_position_t position, memory_sample_count_t &samples_read_from_decoder){
	audio_buffer_t buffer = this->decoder.read_more(position);
	if (!buffer || this->dont_convert)
		return buffer;
	memory_sample_count_t samples_required = samples_read_from_decoder = buffer.samples();
	for (size_t i = 0; i < this->filters.size(); i++)
		samples_required = this->filters[i]->calculate_required_size(samples_required);
	audio_buffer_t ret = buffer.clone_with_minimum_length(samples_required);
	for (size_t i = 0; i < this->filters.size(); i++)
		this->filters[i]->read(ret);
	return ret;
}
