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
#include <cmath>
#endif

std::wstring OggMetadata::ALBUM                  = L"ALBUM";
std::wstring OggMetadata::ARTIST                 = L"ARTIST";
std::wstring OggMetadata::DATE                   = L"DATE";
std::wstring OggMetadata::METADATA_BLOCK_PICTURE = L"METADATA_BLOCK_PICTURE";
std::wstring OggMetadata::OPUS                   = L"OPUS";
std::wstring OggMetadata::PART                   = L"PART";
std::wstring OggMetadata::TITLE                  = L"TITLE";
std::wstring OggMetadata::TRACKNUMBER            = L"TRACKNUMBER";
static std::wstring REPLAYGAIN_TRACK_GAIN        = L"REPLAYGAIN_TRACK_GAIN";
static std::wstring REPLAYGAIN_TRACK_PEAK        = L"REPLAYGAIN_TRACK_PEAK";
static std::wstring REPLAYGAIN_ALBUM_GAIN        = L"REPLAYGAIN_ALBUM_GAIN";
static std::wstring REPLAYGAIN_ALBUM_PEAK        = L"REPLAYGAIN_ALBUM_PEAK";
static const std::wstring * const replaygain_strings[] = {
	&REPLAYGAIN_TRACK_GAIN,
	&REPLAYGAIN_TRACK_PEAK,
	&REPLAYGAIN_ALBUM_GAIN,
	&REPLAYGAIN_ALBUM_PEAK,
};

static bool is_replaygain_tag(const std::wstring &s){
	for (auto p : replaygain_strings)
		if (!strcmp_case(s, *p))
			return 1;
	return 0;
}

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
	toupper_inplace(field_name);
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

template <typename T>
bool find_in_map(const T &map, const std::wstring &what, double &dst){
	auto it = map.find(what);
	if (it == map.end())
		return 0;
	return !!(std::wstringstream(it->second) >>dst);
}

bool OggMetadata::track_gain(double &dst){
	return find_in_map(this->map, REPLAYGAIN_TRACK_GAIN, dst);
}

bool OggMetadata::track_peak(double &dst){
	return find_in_map(this->map, REPLAYGAIN_TRACK_PEAK, dst);
}

bool OggMetadata::album_gain(double &dst){
	return find_in_map(this->map, REPLAYGAIN_ALBUM_GAIN, dst);
}

bool OggMetadata::album_peak(double &dst){
	return find_in_map(this->map, REPLAYGAIN_ALBUM_PEAK, dst);
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
	if (is_replaygain_tag(key))
		toupper_inplace(key);
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

bool Mp3Metadata::track_gain(double &dst){
	return find_in_map(this->texts, REPLAYGAIN_TRACK_GAIN, dst);
}

bool Mp3Metadata::track_peak(double &dst){
	return find_in_map(this->texts, REPLAYGAIN_TRACK_PEAK, dst);
}

bool Mp3Metadata::album_gain(double &dst){
	return find_in_map(this->texts, REPLAYGAIN_ALBUM_GAIN, dst);
}

bool Mp3Metadata::album_peak(double &dst){
	return find_in_map(this->texts, REPLAYGAIN_ALBUM_PEAK, dst);
}

inline double db_to_multiplier(double db){
	return pow(10.0, db / 20.0);
}

double replaygain_get_multiplier(GenericMetadata &meta, const ReplayGainSettings &settings){
	double gain,
		peak;
	int gain_present = -1,
		peak_present = -1;
	double values[2][2];
	bool values_present[2][2];
	typedef bool (GenericMetadata::*f_t)(double &);
	static f_t functions[] = {
		&GenericMetadata::album_gain,
		&GenericMetadata::album_peak,
		&GenericMetadata::track_gain,
		&GenericMetadata::track_peak,
	};
	unsigned index = settings.prefer_album_gain ? 0 : 2;
	{
		int i = 0;
		for (auto p : functions){
			auto x = index / 2;
			auto y = index % 2;
			values_present[x][y] = (meta.*functions[i++])(values[x][y]);
			index++;
		}
	}
	
	if (values_present[0][0]){
		gain_present = 0;
		gain = values[0][0];
	}else if (values_present[1][0]){
		gain_present = 1;
		gain = values[1][0];
	}

	if (gain_present < 0)
		return db_to_multiplier(settings.fallback_preamp_db);

	if (values_present[gain_present][1]){
		peak_present = gain_present;
		peak = values[gain_present][1];
	}

	if (settings.apply_preamp)
		gain += settings.preamp_db;

	double ret = db_to_multiplier(gain);
	if (settings.apply_peak && peak_present >= 0)
		ret = std::min(ret, 1.0 / peak);

	return ret;
}
