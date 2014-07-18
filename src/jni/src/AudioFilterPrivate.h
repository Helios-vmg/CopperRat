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

#include "AudioFilter.h"

class AudioFilter{
protected:
	AudioFormat src_format;
	AudioFormat dst_format;
public:
	AudioFilter(const AudioFormat &src_format, const AudioFormat &dst_format): src_format(src_format), dst_format(dst_format){}
	virtual ~AudioFilter(){}
	virtual void read(audio_buffer_t *buffers, size_t size) = 0;
	virtual size_t calculate_required_byte_size(size_t) = 0;
	const AudioFormat &get_src_format() const{
		return this->src_format;
	}
};

#define INTEGER_TYPE_LIST \
	NODE(Sint16) +        \
	NODE(Sint8)  +        \
	NODE(Sint32) +        \
	NODE(Uint16) +        \
	NODE(Uint8)  +        \
	NODE(Uint32) +        \
	NODE(Sint64) +        \
	NODE(Uint64)

class SignednessFilter : public AudioFilter{
public:
	SignednessFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	static SignednessFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
	size_t calculate_required_byte_size(size_t bytes){
		return bytes;
	}
	template <typename DstT, typename SrcT>
	static SignednessFilter *create_helper(const AudioFormat &src_format, const AudioFormat &dst_format);
};

class BitShiftingFilter : public AudioFilter{
public:
	BitShiftingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	static BitShiftingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
	size_t calculate_required_byte_size(size_t bytes){
		return bytes * this->dst_format.bytes_per_channel / this->src_format.bytes_per_channel;
	}
	
	template <typename DstT, typename SrcT>
	static BitShiftingFilter *create_helper(const AudioFormat &src_format, const AudioFormat &dst_format);
};

class ChannelMixingFilter : public AudioFilter{
public:
	ChannelMixingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	static ChannelMixingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
	size_t calculate_required_byte_size(size_t bytes){
		return bytes * this->dst_format.channels / this->src_format.channels;
	}
};

class MultiplicationFilter : public AudioFilter{
	double factor;
	Uint16 multiplier,
		divider;
public:
	MultiplicationFilter(const AudioFormat &format, double scaling_factor);
	~MultiplicationFilter(){}
	static MultiplicationFilter *create(const AudioFormat &format, double scaling_factor){
		return new MultiplicationFilter(format, scaling_factor);
	}
	void read(audio_buffer_t *buffers, size_t size);
	size_t calculate_required_byte_size(size_t bytes){
		return bytes;
	}
};

class ResamplingFilter : public AudioFilter{
public:
	ResamplingFilter(const AudioFormat &src_format, const AudioFormat &dst_format): AudioFilter(src_format, dst_format){}
	virtual ~ResamplingFilter(){}
	static ResamplingFilter *create(const AudioFormat &src_format, const AudioFormat &dst_format);
	size_t calculate_required_byte_size(size_t bytes){
		return bytes * this->dst_format.freq / this->src_format.freq;
	}
};

#include "TypeList.h"

template <typename ResultT, typename Test, typename Dst = TypeNil>
class CreatorFunctor{
	unsigned step;
	AudioFormat dst, src;
	Test test;
	ResultT *result;
public:
	CreatorFunctor(const AudioFormat &dst, const AudioFormat &src): step(0), dst(dst), src(src), result(0){}
	template <typename OtherT>
	CreatorFunctor(const CreatorFunctor<ResultT, Test, OtherT> &cf): step(cf.get_step() + 1), dst(cf.get_dst()), src(cf.get_src()), result(0){}
	ResultT *get_result() const{
		return this->result;
	}
	template <typename T, typename ListType>
	bool operator()(const T &, const ListType &list){
		const AudioFormat &af = !this->step ? this->dst : this->src;
		if (this->test(T(), af)){
			if (!this->step){
				CreatorFunctor<ResultT, Test, T> cf(*this);
				iterate_type_list(list, cf);
				this->result = cf.get_result();
			}else
				this->result = ResultT::template create_helper<Dst, T>(this->src, this->dst);
			return 0;
		}
		return 1;
	}
	unsigned get_step() const{
		return this->step;
	}
	const AudioFormat &get_dst() const{
		return this->dst;
	}
	const AudioFormat &get_src() const{
		return this->src;
	}
};
