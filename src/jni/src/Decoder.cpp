#include "OggDecoder.h"
#include "FlacDecoder.h"
#include "CommonFunctions.h"
#include <string>

Decoder *Decoder::create(AudioStream &stream, const char *filename){
	std::string ext = filename;
	auto dot = ext.rfind('.');
	if (dot == ext.npos)
		return 0;
	ext = ext.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
	if (ext == "ogg")
		return new OggDecoder(stream, filename);
	if (ext == "flac")
		return new FlacDecoder(stream, filename);
	return 0;
}

audio_buffer_t Decoder::read(){
	audio_buffer_t ret = this->read_more_internal();
	if (!ret)
		return ret;
	this->current_position += ret.samples();
	return ret;
}
