#include "Mp3Decoder.h"
#include "AudioStream.h"
#include <fstream>
#include <boost/type_traits.hpp>

typedef boost::make_signed<size_t>::type my_ssize_t;
#define ssize_t my_ssize_t
#include <mpg123.h>

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

Mp3Decoder::Mp3Decoder(AudioStream &parent, const char *path): Decoder(parent){
	this->has_played = 0;
	this->handle = 0;
	this->fd = -1;
	int error;

	if (!static_data.init())
		throw DecoderInitializationException();

	this->handle = mpg123_new(0, &error);
	if (!this->handle)
		throw DecoderInitializationException();

	error = mpg123_replace_reader(this->handle, mp3_read, mp3_seek);
	if (error != MPG123_OK)
		throw DecoderInitializationException();

	boost::shared_ptr<std::ifstream> stream(new std::ifstream(path, std::ios::binary));
	error = mpg123_open_fd(this->handle, this->fd = tracker.add(stream));
	if (error != MPG123_OK)
		throw DecoderInitializationException();

	error = mpg123_format_none(this->handle);
	if (error != MPG123_OK)
		throw DecoderInitializationException();

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

	error = mpg123_scan(this->handle);
	if (error != MPG123_OK)
		return;
	this->length = mpg123_length(this->handle);
	this->seconds_per_frame = mpg123_tpf(this->handle);
}

Mp3Decoder::~Mp3Decoder(){
	if (this->handle){
		mpg123_close(this->handle);
		mpg123_delete(this->handle);
	}
	tracker.remove(this->fd);
}

void Mp3Decoder::set_format(){
	int error = mpg123_format(this->handle, this->freq, this->channels_enum, this->encoding_enum);
	if (error != MPG123_OK)
		throw DecoderException();
}

audio_buffer_t Mp3Decoder::read_more_internal(){
	unsigned char *buffer;
	off_t offset;
	size_t size;
	
	//this->set_format();

	int error = mpg123_decode_frame(this->handle, &offset, &buffer, &size);
	if (error == MPG123_DONE)
		return audio_buffer_t();
	if (error != MPG123_OK && error != MPG123_NEW_FORMAT)
		throw DecoderException();

	audio_buffer_t ret(this->format.bytes_per_channel, this->format.channels, size / (this->format.bytes_per_sample()));
	memcpy(ret.raw_pointer(0), buffer, size);
	return ret;
}

double Mp3Decoder::get_seconds_length_internal(){
	return (double)this->length / (double)this->freq;
}

bool Mp3Decoder::seek(audio_position_t pos){
	return mpg123_seek(this->handle, pos, SEEK_SET) >= 0;
}

bool Mp3Decoder::fast_seek_seconds(double seconds, audio_position_t &new_position){
	off_t frame = mpg123_timeframe(this->handle, seconds);
	off_t result = mpg123_seek_frame(this->handle, frame, SEEK_SET);
	bool ret = result >= 0;
	if (ret)
		new_position = result;
	return ret;
}
