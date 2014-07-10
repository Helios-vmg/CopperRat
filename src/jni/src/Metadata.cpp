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
#include "Metadata.h"
#include "CommonFunctions.h"
#include "base64.h"
#include "my_mpg123.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#endif

std::wstring OggMetadata::ALBUM                  = L"ALBUM";
std::wstring OggMetadata::ARTIST                 = L"ARTIST";
std::wstring OggMetadata::DATE                   = L"DATE";
std::wstring OggMetadata::METADATA_BLOCK_PICTURE = L"METADATA_BLOCK_PICTURE";
std::wstring OggMetadata::OPUS                   = L"OPUS";
std::wstring OggMetadata::PART                   = L"PART";
std::wstring OggMetadata::TITLE                  = L"TITLE";
std::wstring OggMetadata::TRACKNUMBER            = L"TRACKNUMBER";

int GenericMetadata::track_number_int(){
	std::wstringstream stream(this->track_number());
	int ret;
	if (!(stream >>ret))
		ret = -1;
	return ret;
}

std::wstring GenericMetadata::track_id(){
	std::wstring ret = this->album();
	ret += L" (";
	ret += this->date();
	ret += L") - ";
	ret += this->track_number();
	ret += L" - ";
	ret += this->track_artist();
	ret += L" - ";
	ret += this->track_title();
	return ret;
}

void OggMetadata::add_vorbis_comment(const void *buffer, size_t length){
	std::wstring comment;
	utf8_to_string(comment, (unsigned char *)buffer, length);
	size_t equals = comment.find('=');
	if (equals == comment.npos)
		//invalid comment
		return;
	auto field_name = comment.substr(0, equals);
	std::transform(field_name.begin(), field_name.end(), field_name.begin(), toupper);
	equals++;
	if (field_name == METADATA_BLOCK_PICTURE){
		std::vector<unsigned char> decoded_buffer;
		b64_decode(decoded_buffer, (const char *)buffer + equals, int(length - equals));
		Uint32 picture_type,
			mime_length,
			description_length,
			picture_width,
			picture_height,
			picture_bit_depth,
			picture_colors,
			picture_size;
		size_t offset = 0;

		if (!read_32_big_bits(picture_type, decoded_buffer, offset) || picture_type != mpg123_id3_pic_front_cover)
			return;
		offset += 4;

		if (!read_32_big_bits(mime_length, decoded_buffer, offset))
			return;
		offset += 4 + mime_length;

		if (!read_32_big_bits(description_length, decoded_buffer, offset))
			return;
		offset += 4 + description_length;

		if (!read_32_big_bits(picture_width, decoded_buffer, offset))
			return;
		offset += 4;

		if (!read_32_big_bits(picture_height, decoded_buffer, offset))
			return;
		offset += 4;

		if (!read_32_big_bits(picture_bit_depth, decoded_buffer, offset))
			return;
		offset += 4;

		if (!read_32_big_bits(picture_colors, decoded_buffer, offset))
			return;
		offset += 4;

		if (!read_32_big_bits(picture_size, decoded_buffer, offset))
			return;
		offset += 4;

		if (decoded_buffer.size() - offset < picture_size)
			return;

		this->ogg_picture.assign(decoded_buffer.begin() + offset, decoded_buffer.begin() + (offset + picture_size));
	}
	auto field_value = comment.substr(equals);
	this->add(field_name, field_value);
}

bool OggMetadata::picture(unsigned char *&buffer, size_t &length){
	if (!this->ogg_picture.size())
		return 0;
	buffer = &this->ogg_picture[0];
	length = this->ogg_picture.size();
	return 1;
}

std::wstring OggMetadata::track_title(){
	std::wstring ret;
	auto i = this->map.find(TITLE);
	if (i == this->map.end())
		return L"";
	ret += i->second;
	i = this->map.find(OPUS);
	if (i != this->map.end()){
		ret += L" (";
		ret += i->second;
		ret += ')';
	}
	i = this->map.find(PART);
	if (i != this->map.end()){
		ret += L", ";
		ret += i->second;
	}
	return ret;
}

std::wstring Mp3Metadata::track_number(){
	auto i = this->texts.find(L"TRCK");
	return i == this->texts.end() ? std::wstring() : i->second;
}

void Mp3Metadata::add_mp3_text(const void *_text){
	auto text = (mpg123_text *)_text;
	std::wstring key, value;
	utf8_to_string(key, (const unsigned char *)text->id, strnlen(text->id, 4));
	utf8_to_string(value, (const unsigned char *)text->text.p, strnlen(text->text.p, text->text.size));
	this->texts[key] = value;
}

void Mp3Metadata::add_mp3_extra(const void *_text){
	auto text = (mpg123_text *)_text;
	std::wstring key, value;
	utf8_to_string(key, (const unsigned char *)text->description.p, strnlen(text->description.p, text->description.size));
	utf8_to_string(value, (const unsigned char *)text->text.p, strnlen(text->text.p, text->text.size));
	this->texts[key] = value;
}

void Mp3Metadata::add_picture(const void *buffer, size_t length){
	this->id3_picture.resize(length);
	memcpy(&this->id3_picture[0], buffer, length);
}

bool Mp3Metadata::picture(unsigned char *&buffer, size_t &length){
	if (!this->id3_picture.size())
		return 0;
	buffer = &this->id3_picture[0];
	length = this->id3_picture.size();
	return 1;
}
