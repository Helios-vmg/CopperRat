#include "FLACDecoder.h"
#include <cassert>

template <typename SampleT>
audio_buffer_t copy_to_new_buffer(const FLAC__Frame *frame, const FLAC__int32 * const *buffer){
	audio_buffer_t ret(frame->header.bits_per_sample / 8, frame->header.channels, frame->header.blocksize);
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

FlacDecoder::FlacDecoder(const char *filename){
	this->set_md5_checking(0);
	this->file.open(filename, std::ios::binary);
	if (!this->file || this->init() != FLAC__STREAM_DECODER_INIT_STATUS_OK)
		throw DecoderInitializationException();
}

void FlacDecoder::free_buffers(){
	this->buffers.clear();
}

bool FlacDecoder::seek(audio_position_t p){
	bool ret = this->seek_absolute(p);
	if (ret)
		this->free_buffers();
	return ret;
}

audio_buffer_t FlacDecoder::read_more(){
	bool ok = 1;
	FLAC::Decoder::Stream::State state = this->get_state();
	while (!this->buffers.size() && (ok = this->process_single()) && (state = this->get_state()) != FLAC__STREAM_DECODER_END_OF_STREAM);
	if (state == FLAC__STREAM_DECODER_END_OF_STREAM || !ok)
		return audio_buffer_t();
	audio_buffer_t ret = this->buffers.front();
	this->buffers.pop_front();
	return ret;
}

FLAC__StreamDecoderWriteStatus FlacDecoder::write_callback(const FLAC__Frame *frame, const FLAC__int32 * const *buffer){
	this->buffers.push_back(allocator_functions[frame->header.bits_per_sample / 8](frame, buffer));
	this->declared_af.bytes_per_channel = this->get_bits_per_sample() / 8;
	this->declared_af.channels = this->get_channels();
	this->declared_af.freq = this->get_sample_rate();
	this->declared_af.is_signed = 1;
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
