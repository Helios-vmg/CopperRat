#include "AudioStream.h"
#include <string>

AudioStream::AudioStream(const char *filename, unsigned frequency, unsigned channels){
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
	this->position = 0;
}

audio_buffer_t AudioStream::read_new(){
	memory_sample_count_t samples_read;
	audio_buffer_t ret = this->filter->read(samples_read);
	if (!ret)
		return ret;
	ret.position = this->position;
	this->position += ret.samples();
#ifdef DUMP_OUTPUT
	this->test_file.write((const char *)ret.raw_pointer(0), ret.byte_length());
#endif
	return ret;
}

#include <iostream>

audio_position_t AudioStream::seek(audio_position_t current_position, float ms){
	audio_position_t target = audio_position_t(current_position + double(ms) * (1.0 / 1000.0) * double(this->decoder->get_audio_format().freq));
	std::cout <<"Current position: "<<current_position<<std::endl
		<<"Seeking to position: "<<target<<std::endl;
	audio_position_t ret = this->decoder->seek(target) ? target : current_position;
	return this->position = ret;
}
