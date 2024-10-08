/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioFilterPrivate.h"
#include "TypeList.h"
#include <type_traits>

#define NODE(x) TypeCons<TypeNil, x>()

template <typename Dst, typename Src>
struct Converter{
	static Dst convert(Src z){
#ifdef _MSC_VER
#pragma warning(push)
//Disable: "integral contant overflow".
#pragma warning(disable: 4307)
//Disable: "shift count negative or too big, undefined behavior".
#pragma warning(disable: 4293)
//Disable: "truncation of constant value".
#pragma warning(disable: 4309)
#endif
		//The lines below may generate warnings in your compiler. They're perfectly safe.
		if (sizeof(Dst) > sizeof(Src)){
			const unsigned shift = (sizeof(Dst) - sizeof(Src)) * CHAR_BIT;
			return (Dst)z << shift;
		}else if (sizeof(Dst) < sizeof(Src)){
			const unsigned shift = (sizeof(Src) - sizeof(Dst)) * CHAR_BIT;
			return (Dst)(z >> shift);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		}else
			return (Dst)z;
		return 0;
	}
};

//Works between any two integer types.
template <typename DstT, typename SrcT>
class BitShiftingFilterGeneric : public BitShiftingFilter{
public:
	BitShiftingFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format): BitShiftingFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){
		audio_buffer_t &buffer = buffers[0];
		const SrcT *src_array = (const SrcT *)buffer.raw_pointer(0);
		const size_t array_size = buffer.samples() * buffer.channels();
		DstT *dst_array = (DstT *)src_array;

		int shift = (this->dst_format.bytes_per_channel - this->src_format.bytes_per_channel) * CHAR_BIT;
		if (shift > 0){
			for (size_t i = array_size; i--;)
				dst_array[i] = (DstT)src_array[i] << shift;
		}else if (shift < 0){
			for (size_t i = 0; i != array_size; i++)
				dst_array[i] = (DstT)(src_array[i] >> -shift);
		}
	}
};

template <typename SrcT>
class BitShiftingFilterGeneric<TypeNil, SrcT> : public BitShiftingFilter{
public:
	BitShiftingFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format): BitShiftingFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){}
};

template <typename DstT, typename SrcT>
BitShiftingFilter *BitShiftingFilter::create_helper(const AudioFormat &src_format, const AudioFormat &dst_format){
	return new BitShiftingFilterGeneric<DstT, SrcT>(src_format, dst_format);
}

template <typename T>
inline size_t alternate_sizeof(const T &){
	return sizeof(T);
}

struct BitShiftingTest{
	template <typename T>
	bool operator()(const T &, const AudioFormat &af) const{
		switch (af.bytes_per_channel){
			case 1:
			case 2:
			case 4:
			case 8:
				return sizeof(T) == af.bytes_per_channel;
			case 3:
				return sizeof(T) == 4;
			case 5:
			case 6:
			case 7:
				return sizeof(T) == 8;
		}
		return sizeof(T) == af.bytes_per_channel;
	}
};

BitShiftingFilter *BitShiftingFilter::create(const AudioFormat &src_format, const AudioFormat &dst_format){
	CreatorFunctor<BitShiftingFilter, BitShiftingTest> cf(dst_format, src_format);
	iterate_type_list(INTEGER_TYPE_LIST, cf);
	return cf.get_result();
}
