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
#include "Mp3Decoder.h"
#include "AudioStream.h"
#include "CommonFunctions.h"
#include "my_mpg123.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <fstream>
#include <boost/type_traits.hpp>
#endif


mp3_static_data Mp3Decoder::static_data;

struct fd_tracker{
	int counter;
	typedef std::map<int, boost::shared_ptr<std::ifstream> > map_t;
	map_t map;
	fd_tracker():counter(0){}
	int add(boost::shared_ptr<std::ifstream>);
	boost::shared_ptr<std::ifstream> get(int);
	void remove(int);
} tracker;

int fd_tracker::add(boost::shared_ptr<std::ifstream> file){
	this->map[this->counter] = file;
	return this->counter++;
}

boost::shared_ptr<std::ifstream> fd_tracker::get(int fd){
	return this->map[fd];
}

void fd_tracker::remove(int fd){
	auto i = this->map.find(fd);
	if (i == this->map.end())
		return;
	this->map.erase(i);
}

my_ssize_t mp3_read(int fd, void *dst, size_t n){
	auto file = tracker.get(fd);
	file->read((char *)dst, n);
	return file->gcount();
}

off_t mp3_seek(int fd, off_t offset, int whence){
	auto file = tracker.get(fd);
	static std::ios::seekdir whences[] = {
		std::ios::beg,
		std::ios::cur,
		std::ios::end
	};
	file->clear();
	file->seekg(offset, whences[whence % 3]);
	return (off_t)file->tellg();
}

bool mp3_static_data::init(){
	return this->initialized ? 1 : this->initialized = MPG123_OK == mpg123_init();
}

mp3_static_data::~mp3_static_data(){
	if (this->initialized)
		mpg123_exit();
}

Mp3Decoder::Mp3Decoder(AudioStream &parent, const std::wstring &path): Decoder(parent, path){
	this->has_played = 0;
	this->handle = 0;
	this->fd = -1;
	int error;

	if (!static_data.init())
		throw DecoderInitializationException("Failed to initialize static data for MP3 decoder.");

	this->handle = mpg123_new(0, &error);
	if (!this->handle)
		throw DecoderInitializationException("MP3 decoder initialization failed.");

	error = mpg123_replace_reader(this->handle, mp3_read, mp3_seek);
	if (error != MPG123_OK)
		throw DecoderInitializationException("MP3 decoder initialization failed.");

	auto converted_path = 
#ifndef _MSC_VER
		string_to_utf8
#endif
		(path);

	boost::shared_ptr<std::ifstream> stream(new std::ifstream(converted_path.c_str(), std::ios::binary));
	error = mpg123_open_fd(this->handle, this->fd = tracker.add(stream));
	if (error != MPG123_OK)
		throw DecoderInitializationException("MP3 read failed.");

	error = mpg123_format_none(this->handle);
	if (error != MPG123_OK)
		throw DecoderInitializationException("mpg123_format_none() failed???");

	auto format = this->format = parent.get_preferred_format();
	static int encodings[] = {
		MPG123_ENC_UNSIGNED_8,
		MPG123_ENC_SIGNED_8,
		MPG123_ENC_UNSIGNED_16,
		MPG123_ENC_SIGNED_16,
		MPG123_ENC_UNSIGNED_24,
		MPG123_ENC_SIGNED_24,
		MPG123_ENC_UNSIGNED_32,
		MPG123_ENC_SIGNED_32,
	};
	this->freq = format.freq;
	this->channels_enum = format.channels == 2 ? MPG123_STEREO : MPG123_MONO;
	this->encoding_enum = encodings[format.bytes_per_channel * ((int)format.is_signed + 1) - 1];
	this->set_format();

	this->length = invalid_sample_count;
	
	long long_param;
	double double_param;
	mpg123_getparam(this->handle, MPG123_FLAGS, &long_param, &double_param);
	long_param |= MPG123_PICTURE;
	mpg123_param(this->handle, MPG123_FLAGS, long_param, double_param);

	error = mpg123_scan(this->handle);
	if (error != MPG123_OK)
		return;
	this->length = mpg123_length(this->handle);
	this->seconds_per_frame = mpg123_tpf(this->handle);

	mpg123_id3(this->handle, (mpg123_id3v1 **)&this->id3v1, (mpg123_id3v2 **)&this->id3v2);
	this->metadata_done = 0;
	this->check_for_metadata();
}

Mp3Decoder::~Mp3Decoder(){
	if (this->handle){
		mpg123_close(this->handle);
		mpg123_delete(this->handle);
	}
	tracker.remove(this->fd);
}

void Mp3Decoder::check_for_metadata(){
	if (this->metadata_done)
		return;
	auto result = mpg123_meta_check(this->handle);
	if (!(result & MPG123_ID3))
		return;
	auto p = (mpg123_id3v2 *)this->id3v2;
	boost::shared_ptr<Mp3Metadata> meta(new Mp3Metadata(this->path));
#define SET_ID3_DATA(x) if (p->x) utf8_to_string(meta->id3_##x, (const unsigned char *)p->x->p, strnlen(p->x->p, p->x->size))
	SET_ID3_DATA(title);
	SET_ID3_DATA(artist);
	SET_ID3_DATA(album);
	SET_ID3_DATA(year);
	SET_ID3_DATA(genre);
	SET_ID3_DATA(comment);
	for (size_t i = 0; i < p->texts; i++)
		meta->add_mp3_text(p->text + i);
	for (size_t i = 0; i < p->extras; i++)
		meta->add_mp3_text(p->extra + i);
	for (size_t i = 0; i < p->pictures; i++){
		if (p->picture[i].type != mpg123_id3_pic_front_cover)
			continue;
		meta->add_picture(p->picture[i].data, p->picture[i].size);
		break;
	}
	this->parent.metadata_update(meta);
	mpg123_meta_free(this->handle);
	this->metadata_done = 1;
}

void Mp3Decoder::set_format(){
	int error = mpg123_format(this->handle, this->freq, this->channels_enum, this->encoding_enum);
	if (error != MPG123_OK)
		throw DecoderException("mpg123_format() failed.");
}

audio_buffer_t Mp3Decoder::read_more_internal(){
	unsigned char *buffer;
	off_t offset;
	size_t size;
	
	//this->set_format();

	int error = mpg123_decode_frame(this->handle, &offset, &buffer, &size);
	this->check_for_metadata();
	if (error == MPG123_DONE)
		return audio_buffer_t();
	if (error != MPG123_OK && error != MPG123_NEW_FORMAT)
		throw DecoderException("Decoding of MP3 frame failed.");

	const auto samples = memory_sample_count_t(size / (this->format.bytes_per_sample()));
	audio_buffer_t ret(this->format.bytes_per_channel, this->format.channels, samples);
	memcpy(ret.raw_pointer(0), buffer, size);
	return ret;
}

double Mp3Decoder::get_seconds_length_internal(){
	return (double)this->length / (double)this->freq;
}

bool Mp3Decoder::seek(audio_position_t pos){
	return mpg123_seek(this->handle, (off_t)pos, SEEK_SET) >= 0;
}

bool Mp3Decoder::fast_seek_seconds(double seconds, audio_position_t &new_position){
	off_t frame = mpg123_timeframe(this->handle, seconds);
	off_t result = mpg123_seek_frame(this->handle, frame, SEEK_SET);
	bool ret = result >= 0;
	if (ret)
		new_position = result;
	return ret;
}
