#include "OggDecoder.h"
#include "CommonFunctions.h"

Decoder *Decoder::create(const char *filename){
	return new OggDecoder(filename);
}

audio_buffer_t Decoder::read_more(audio_position_t initial_position){
	audio_buffer_t ret;
	if (this->current_position != initial_position && !this->seek(initial_position))
		return ret;
	ret = this->read_more();
	if (!ret)
		return ret;
	this->current_position += ret.samples();
	return ret;
}
