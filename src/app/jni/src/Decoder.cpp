/*

Copyright (c) 2014, Helios
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
