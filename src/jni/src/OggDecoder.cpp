#include "OggDecoder.h"

OggDecoder::OggDecoder(const char *filename){
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
	this->file = fopen(filename, "rb");
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	if (!this->file)
		throw DecoderInitializationException();
	this->bitstream = 0;
	ov_callbacks cb;
	cb.read_func = OggDecoder::read;
	cb.seek_func = OggDecoder::seek;
	cb.tell_func = OggDecoder::tell;
	cb.close_func = 0;
	int error = ov_open_callbacks(this, &this->ogg_file, 0, 0, cb);
	if (error < 0)
		throw DecoderInitializationException();
	vorbis_info *i = ov_info(&this->ogg_file, this->bitstream);
	this->frequency = i->rate;
	this->channels = i->channels;
}

OggDecoder::~OggDecoder(){
	ov_clear(&this->ogg_file);
	if (this->file)
		fclose(this->file);
}

const char *ogg_code_to_string(int e){
	switch (e){
		case 0:
			return "no error";
		case OV_EREAD:
			return "a read from media returned an error";
		case OV_ENOTVORBIS:
			return "bitstream does not contain any Vorbis data";
		case OV_EVERSION:
			return "vorbis version mismatch";
		case OV_EBADHEADER:
			return "invalid Vorbis bitstream header";
		case OV_EFAULT:
			return "internal logic fault; indicates a bug or heap/stack corruption";
		default:
			return "unknown error";
	}
}

audio_buffer_t OggDecoder::read_more_internal(){
	const size_t samples_to_read = 1024;
	const size_t bytes_per_sample = this->channels*2;
	const size_t bytes_to_read = samples_to_read*bytes_per_sample;
	audio_buffer_t ret(sizeof(Sint16), this->channels, samples_to_read);
	size_t size = 0;
	while (size < bytes_to_read){
		int r = ov_read(&this->ogg_file, ((char *)ret.get_sample_use_channels<Sint16>(0)) + size, int(bytes_to_read - size), 0, 2, 1, &this->bitstream);
		if (r < 0)
			abort();
		if (!r){
			if (!size)
				ret.unref();
			break;
		}
		size += r;
	}
	ret.set_sample_count(unsigned(size / bytes_per_sample));
	return ret;
}

bool OggDecoder::seek(audio_position_t pos){
	return !ov_pcm_seek(&this->ogg_file, pos);
}

bool OggDecoder::fast_seek(audio_position_t pos){
	return !ov_pcm_seek_page(&this->ogg_file, pos);
}

size_t OggDecoder::read(void *buffer, size_t size, size_t nmemb, void *s){
	OggDecoder *_this = (OggDecoder *)s;
	size_t ret = fread(buffer, size, nmemb, _this->file);
	return ret;
}

int OggDecoder::seek(void *s, ogg_int64_t offset, int whence){
	OggDecoder *_this = (OggDecoder *)s;
	int ret = fseek(_this->file, (long)offset, whence);
	return ret;
}

long OggDecoder::tell(void *s){
	OggDecoder *_this = (OggDecoder *)s;
	long ret = ftell(_this->file);
	return ret;
}