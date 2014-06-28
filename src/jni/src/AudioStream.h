#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H
#include "AudioFilter.h"
#include <memory>
#include <boost/shared_ptr.hpp>
#include "auto_ptr.h"
//#define DUMP_OUTPUT

#ifdef DUMP_OUTPUT
#include <fstream>
#endif

class AudioPlayer;

class AudioStream{
	AudioPlayer &parent;
	CR_UNIQUE_PTR(Decoder) decoder;
	CR_UNIQUE_PTR(AudioFilterManager) filter;
	audio_position_t position;
	std::wstring path;
#ifdef DUMP_OUTPUT
	std::ofstream test_file;
#endif
	AudioFormat dst_format;
public:
	AudioStream(AudioPlayer &parent, const std::wstring &path, unsigned frequency, unsigned channels);
	audio_buffer_t read();
	bool reset();
	void seek(AudioPlayer *player, audio_position_t &new_position, audio_position_t current_position, double seconds);
	double get_total_time(){
		return this->decoder->get_seconds_length();
	}
	void metadata_update(boost::shared_ptr<GenericMetadata>);
	AudioFormat get_preferred_format() const{
		return this->dst_format;
	}
	const std::wstring &get_path() const{
		return this->path;
	}
};

#endif
