#include "OggDecoder.h"
#include "FlacDecoder.h"
#include "Mp3Decoder.h"
#include "CommonFunctions.h"
#include <string>

Decoder *Decoder::create(AudioStream &stream, const std::wstring &path){
	std::wstring ext = path;
	auto dot = ext.rfind('.');
	if (dot == ext.npos)
		return 0;
	ext = ext.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
	if (ext == L"ogg")
		return new OggDecoder(stream, path);
	if (ext == L"mp3")
		return new Mp3Decoder(stream, path);
	if (ext == L"flac")
		return new FlacDecoder(stream, path);
	return 0;
}

audio_buffer_t Decoder::read(){
	audio_buffer_t ret = this->read_more_internal();
	if (!ret)
		return ret;
	this->current_position += ret.samples();
	return ret;
}
