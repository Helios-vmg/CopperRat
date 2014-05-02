#include "OggDecoder.h"
#include "CommonFunctions.h"

Decoder *Decoder::create(const char *filename){
	return new OggDecoder(filename);
}

const sample_t *Decoder::operator[](audio_position_t position){
	bool below;
	const sample_t *ret;
	do
		ret = this->buffers.get_sample(position, below);
	while (!ret && !below && this->read_next());
	return ret;
}

sample_count_t Decoder::direct_output(audio_buffer_t buffer, audio_position_t sample){
	sample_count_t samples_read = 0;
	while (buffer.sample_count){
		bool below;
		unsigned call_result = this->buffers.copy_buffer(buffer, sample, below);
		if (!call_result){
			if (below)
				return 0;
			if (!this->read_next())
				break;
			continue;
		}
		samples_read += call_result;
		buffer.sample_count -= call_result;
		sample += call_result;
		buffer.data = buffer[call_result];
	}
	return samples_read;
}

audio_buffer_t remix_channels(audio_buffer_t buffer){
	switch (buffer.channel_count){
		case 1:
			{
				audio_buffer_t temp = audio_buffer_t::alloc(2, buffer.sample_count);
				temp.samples_produced = buffer.samples_produced;
				for (sample_count_t i = 0; i < temp.sample_count; i++){
					temp[(size_t)i][1] =
					temp[(size_t)i][0] = buffer[(size_t)i][0];
				}
				buffer.free();
				buffer = temp;
			}
			break;
		case 2:
			break;
		default:
			{
				double multiplier = 1.0 / double(buffer.channel_count);
				audio_buffer_t temp = audio_buffer_t::alloc(2, buffer.sample_count);
				temp.samples_produced = buffer.samples_produced;
				for (sample_count_t i = 0; i < temp.sample_count; i++){
					double avg = 0;
					sample_t *sample = buffer[(size_t)i];
					for (unsigned channel = 0; channel < buffer.channel_count; channel++)
						avg += s16_to_double(sample[channel]);
					avg *= multiplier;
					temp[(size_t)i][1] =
					temp[(size_t)i][0] = double_to_s16(avg);
				}
				buffer.free();
				buffer=temp;
			}
			break;
	}
	return buffer;
}

bool Decoder::read_next(){
	audio_buffer_t buffer = this->read_more();
	if (!buffer.data)
		return 0;
	
	this->buffers.push(remix_channels(buffer));
	return 1;
}
