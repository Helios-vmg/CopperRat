#include "AudioStream.h"

AudioStream::AudioStream(const char *filename, unsigned frequency, unsigned channels, unsigned dfl){
	this->frequency = frequency;
	this->channels = channels;
	this->default_buffer_length = dfl;
	this->position = 0;
	this->decoder.reset(Decoder::create(filename));
	AudioFormat dst_format = { channels, 2, frequency };
	filter.reset(new AudioFilterManager(*this->decoder, dst_format));
	if (!this->decoder.get())
		return;
}

audio_buffer_t AudioStream::read_new(){
	sample_count_t samples_read;
	audio_buffer_t ret = this->filter->read(this->position, samples_read);
	if (!ret)
		return ret;
	this->position += samples_read;
	return ret;
}
