#include "AudioStream.h"
#include <string>

AudioStream::AudioStream(const char *filename, unsigned frequency, unsigned channels, unsigned dfl){
	this->frequency = frequency;
	this->channels = channels;
	this->default_buffer_length = dfl;
	this->position = 0;
	this->decoder.reset(Decoder::create(filename));
	if (!this->decoder.get())
		return;
	AudioFormat dst_format(true, 2, channels, frequency);
	filter.reset(new AudioFilterManager(*this->decoder, dst_format));
#ifdef DUMP_OUTPUT
	std::string s = filename;
	s.append(".raw");
	this->test_file.open(s.c_str(), std::ios::binary);
#endif
}

audio_buffer_t AudioStream::read_new(){
	memory_sample_count_t samples_read;
	audio_buffer_t ret = this->filter->read(this->position, samples_read);
	if (!ret)
		return ret;
#ifdef DUMP_OUTPUT
	this->test_file.write((const char *)ret.raw_pointer(0), ret.byte_length());
#endif
	this->position += samples_read;
	return ret;
}
