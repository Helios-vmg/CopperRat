/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "FLACDecoder.h"
#include "CommonFunctions.h"
#include "AudioStream.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <cassert>
#include <utility>
#include <vector>
#include <memory>
#endif

const char *to_string(FLAC__StreamDecoderInitStatus status){
	switch (status){
		case FLAC__STREAM_DECODER_INIT_STATUS_OK:
			return "OK";
		case FLAC__STREAM_DECODER_INIT_STATUS_UNSUPPORTED_CONTAINER:
			return "UNSUPPORTED_CONTAINER";
		case FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS:
			return "INVALID_CALLBACKS";
		case FLAC__STREAM_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR:
			return "MEMORY_ALLOCATION_ERROR";
		case FLAC__STREAM_DECODER_INIT_STATUS_ERROR_OPENING_FILE:
			return "ERROR_OPENING_FILE";
		case FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED:
			return "ALREADY_INITIALIZED";
		default:
			return "unknown error";
	}
}

FlacDecoder::FlacDecoder(AudioStream &parent, const std::wstring &path):
		Decoder(parent, path),
		metadata(path),
		declared_af_set(0){
	this->set_md5_checking(0);

	auto converted_path = 
#ifndef _MSC_VER
		string_to_utf8
#endif
		(path);

	this->file.open(converted_path.c_str(), std::ios::binary);
	this->set_metadata_respond_all();
	if (!this->file)
		throw FileNotFoundException(string_to_utf8(path));
	auto status = this->init();
	if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
		throw DecoderInitializationException(std::string("FLAC initialization failed: ") + to_string(status));
	this->declared_af.freq = 0;
	this->process_until_end_of_metadata();
}

template <typename SampleT>
audio_buffer_t copy_to_new_buffer(const FLAC__Frame *frame, const FLAC__int32 * const *buffer){
	audio_buffer_t ret(sizeof(SampleT), frame->header.channels, frame->header.blocksize);
	SampleT *array = (SampleT *)ret.raw_pointer(0);
	size_t array_size = ret.byte_length();
	unsigned channels = ret.channels();
	memory_sample_count_t samples = ret.samples();
	for (unsigned channel = 0; channel < channels; channel++){
		for (memory_audio_position_t sample = 0; sample < samples; sample++){
			const size_t position = sample * channels + channel;
			assert((position + 1) * sizeof(SampleT) <= array_size);
			array[position] = buffer[channel][sample];
		}
	}
	return ret;
}

FlacDecoder::allocator_func FlacDecoder::allocator_functions[] = {
	0,
	copy_to_new_buffer<Sint8>,
	copy_to_new_buffer<Sint16>,
	copy_to_new_buffer<Sint32>,
	copy_to_new_buffer<Sint32>,
};

void FlacDecoder::free_buffers(){
	this->buffers.clear();
}

bool FlacDecoder::seek(audio_position_t p){
	bool ret = this->seek_absolute(p);
	if (ret)
		this->free_buffers();
	return ret;
}

audio_buffer_t FlacDecoder::read_more_internal(){
	bool ok = 1;
	FLAC::Decoder::Stream::State state = this->get_state();
	while (!this->buffers.size() && (ok = this->process_single()) && (state = this->get_state()) != FLAC__STREAM_DECODER_END_OF_STREAM);
	if (state == FLAC__STREAM_DECODER_END_OF_STREAM || !ok)
		return audio_buffer_t();
	audio_buffer_t ret = this->buffers.front();
	this->buffers.pop_front();
	return ret;
}

sample_count_t FlacDecoder::get_pcm_length_internal(){
	return this->get_total_samples();
}

double FlacDecoder::get_seconds_length_internal(){
	if (!this->declared_af_set)
		return -1;
	return (double)this->get_pcm_length_internal() / this->declared_af.freq;
}

FLAC__StreamDecoderWriteStatus FlacDecoder::write_callback(const FLAC__Frame *frame, const FLAC__int32 * const *buffer){
	this->buffers.push_back(allocator_functions[frame->header.bits_per_sample / 8](frame, buffer));
	this->set_af();
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderReadStatus FlacDecoder::read_callback(FLAC__byte *buffer,size_t *bytes){
	if (!this->file)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	this->file.read((char *)buffer, *bytes);
	*bytes = this->file.gcount();
	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus FlacDecoder::seek_callback(FLAC__uint64 absolute_byte_offset){
	if (!this->file)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	this->file.clear();
	this->file.seekg((std::ios::pos_type)absolute_byte_offset);
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

bool FlacDecoder::eof_callback(){
	return !this->file || this->file.eof();
}

FLAC__StreamDecoderTellStatus FlacDecoder::tell_callback(FLAC__uint64 *absolute_byte_offset){
	if (!this->file)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	*absolute_byte_offset = this->file.tellg();
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus FlacDecoder::length_callback(FLAC__uint64 *stream_length){
	if (!this->file)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	auto saved = this->file.tellg();
	this->file.seekg(0, std::ios::end);
	*stream_length = this->file.tellg();
	this->file.seekg(saved);
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

void FlacDecoder::metadata_callback(const FLAC__StreamMetadata *metadata){
	switch (metadata->type){
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			this->read_vorbis_comments(metadata->data.vorbis_comment);
			break;
		case FLAC__METADATA_TYPE_STREAMINFO:
			{
				const auto &stream_info = metadata->data.stream_info;
				this->declared_af.bytes_per_channel = stream_info.bits_per_sample / 8;
				this->declared_af.channels = stream_info.channels;
				this->declared_af.freq = stream_info.sample_rate;
				this->declared_af.is_signed = 1;
				this->declared_af_set = !!this->declared_af.freq;
			}
			break;
		case FLAC__METADATA_TYPE_PICTURE:
			this->metadata.set_picture(metadata->data.picture.data, metadata->data.picture.data_length);
			this->parent.metadata_update(this->metadata.clone());
			break;
	}
}

void FlacDecoder::read_vorbis_comments(const FLAC__StreamMetadata_VorbisComment &comments){
	for (auto i = comments.num_comments; i--;)
		this->metadata.add_vorbis_comment(comments.comments[i].entry, comments.comments[i].length);
	this->parent.metadata_update(this->metadata.clone());
}

void FlacDecoder::set_af(){
	this->declared_af.bytes_per_channel = this->get_bits_per_sample() / 8;
	this->declared_af.channels = this->get_channels();
	this->declared_af.freq = this->get_sample_rate();
	this->declared_af.is_signed = 1;
	this->declared_af_set = !!this->declared_af.freq;

}
