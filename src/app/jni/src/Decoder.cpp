/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "OggDecoder.h"
#include "FlacDecoder.h"
#include "Mp3Decoder.h"
#include "CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#endif

typedef Decoder *(*create_f)(AudioStream &, const std::wstring &);

template <typename T>
static Decoder *alloc_f(AudioStream &stream, const std::wstring &path){
	return new T(stream, path);
}

static const wchar_t *extensions[] = {
	L"ogg",
	L"mp3",
	L"flac",
};

static create_f functions[] = {
	alloc_f<OggDecoder>,
	alloc_f<Mp3Decoder>,
	alloc_f<FlacDecoder>,
};

Decoder *Decoder::create(AudioStream &stream, const std::wstring &path){
	auto ext = get_extension(path);
	int i = 0;
	for (auto p : extensions){
		if (ext == p)
			return functions[i](stream, path);
		i++;
	}
	return nullptr;
}

audio_buffer_t Decoder::read(){
	audio_buffer_t ret = this->read_more_internal();
	if (!ret)
		return ret;
	this->current_position += ret.samples();
	return ret;
}

void filter_list_by_supported_formats(std::vector<std::wstring> &dst, const std::vector<std::wstring> &src){
	dst.clear();
	for (const auto &s : src)
		if (format_is_supported(s))
			dst.push_back(s);
}

bool format_is_supported(const std::wstring &s){
	auto ext = get_extension(s);
	for (auto p : extensions){
		if (ext == p)
			return 1;
	}
	return 0;
}
