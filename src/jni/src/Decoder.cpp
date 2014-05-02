#include "OggDecoder.h"
#include "CommonFunctions.h"

Decoder *Decoder::create(const char *filename){
	return new OggDecoder(filename);
}

audio_buffer_t Decoder::read_more(audio_position_t initial_position){
	if (this->current_position != initial_position && !this->seek(initial_position))
		return weak_audio_buffer_t();
	return this->read_more();
}
