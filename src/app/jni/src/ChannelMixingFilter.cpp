/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioFilterPrivate.h"

class ChannelMixingOneToTwo : public ChannelMixingFilter{
public:
	ChannelMixingOneToTwo(const AudioFormat &src_format, const AudioFormat &dst_format): ChannelMixingFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){
		audio_buffer_t &buffer = buffers[0];
		auto src = buffer.get_sample<Sint16, 1>(0);
		auto dst = buffer.get_sample<Sint16, 2>(0);
		const memory_sample_count_t samples = buffer.samples();
		for (memory_sample_count_t i = samples; i--;)
			dst[i].values[0] = dst[i].values[1] = src[i].values[0];
	}
};

class ChannelMixingTwoToOne : public ChannelMixingFilter{
public:
	ChannelMixingTwoToOne(const AudioFormat &src_format, const AudioFormat &dst_format): ChannelMixingFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){
		audio_buffer_t &buffer = buffers[0];
		auto src = buffer.get_sample<Sint16, 2>(0);
		auto dst = buffer.get_sample<Sint16, 1>(0);
		const memory_sample_count_t samples = buffer.samples();
		for (memory_sample_count_t i = 0; i != samples; i++){
			Sint32 temp = src[i].values[0] + src[i].values[1];
			dst[i].values[0] = Sint16(temp / 2);
		}
	}
};

class ChannelMixingManyToOne : public ChannelMixingFilter{
public:
	ChannelMixingManyToOne(const AudioFormat &src_format, const AudioFormat &dst_format): ChannelMixingFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){
		audio_buffer_t &buffer = buffers[0];
		auto dst = buffer.get_sample<Sint16, 1>(0);
		const memory_sample_count_t samples = buffer.samples();
		for (memory_sample_count_t i = 0; i != samples; i++){
			auto src = *buffer.get_sample_use_channels<Sint16>(i);
			Sint32 temp = src.values[0] + src.values[1];
			dst[i].values[0] = Sint16(temp / 2);
		}
	}
};

class ChannelMixingManyToTwo : public ChannelMixingFilter{
public:
	ChannelMixingManyToTwo(const AudioFormat &src_format, const AudioFormat &dst_format): ChannelMixingFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){
		audio_buffer_t &buffer = buffers[0];
		auto dst = buffer.get_sample<Sint16, 2>(0);
		const memory_sample_count_t samples = buffer.samples();
		for (memory_sample_count_t i = 0; i != samples; i++){
			auto src = *buffer.get_sample_use_channels<Sint16>(i);
			dst[i].values[0] = src.values[0];
			dst[i].values[1] = src.values[1];
		}
	}
};

ChannelMixingFilter *ChannelMixingFilter::create(const AudioFormat &src_format, const AudioFormat &dst_format){
	switch (dst_format.channels){
		case 1:
			{
				switch (src_format.channels){
					case 1:
						return 0;
					case 2:
						return new ChannelMixingTwoToOne(src_format, dst_format);
					default:
						return new ChannelMixingManyToOne(src_format, dst_format);
				}
			}
		case 2:
			{
				switch (src_format.channels){
					case 1:
						return new ChannelMixingOneToTwo(src_format, dst_format);
					case 2:
						return 0;
					default:
						return new ChannelMixingManyToTwo(src_format, dst_format);
				}
			}
		default:
			return 0;
	}
}

