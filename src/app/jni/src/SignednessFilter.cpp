/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioFilterPrivate.h"
#include "TypeList.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_stdinc.h>
#include <limits>
#include <cassert>
#include <type_traits>
#endif

#define NODE(x) TypeCons<TypeNil, x>()

//Works between any two integer types.
template <typename DstT, typename SrcT>
class SignednessFilterGeneric : public SignednessFilter{
public:
	SignednessFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format): SignednessFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){
		audio_buffer_t &buffer = buffers[0];
		const SrcT *src_array = (const SrcT *)buffer.raw_pointer(0);
		const size_t array_size = buffer.byte_length() / sizeof(SrcT);
		DstT *dst_array = (DstT *)src_array;
		assert(std::is_signed<DstT>::value != std::is_signed<SrcT>::value);
		if (std::is_signed<SrcT>::value){
			for (size_t i = 0; i != array_size; i++)
				dst_array[i] = (DstT)src_array[i] + (DstT)std::numeric_limits<SrcT>::min();
		}else{
			for (size_t i = 0; i != array_size; i++){
				SrcT x = src_array[i];
				const DstT min = std::numeric_limits<DstT>::min();
				const DstT max = std::numeric_limits<DstT>::max();
				const SrcT bigmax = std::numeric_limits<SrcT>::max();
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4146)
#endif
				const DstT neg_max = -max;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
				//This if should be eliminated at compile time.
				if (min == neg_max){
					//The easy case first:
					if (x <= (SrcT)max)
						dst_array[i] = (DstT)x + min;
					else
						dst_array[i] = (DstT)(x - (SrcT)min);
				}else if (min + 1 == neg_max){
					//two's complement case:
					//min + 1 == -max <=> max == -min - 1
					
					if (x <= (SrcT)max)
						dst_array[i] = (DstT)x + min;
					else
						//x > max => x > -min - 1 => x - (SrcT)min > -min - 1 - (SrcT)min
						//By C++ conversion rules: (SrcT)min == max + 1
						//x - (SrcT)min > -min - 1 - (max + 1)
						//x - (SrcT)min > -min - 1 - (-min - 1 + 1)
						//x - (SrcT)min > -min - 1 - (-min)
						//x - (SrcT)min > -min - 1 + min
						//x - (SrcT)min > -1
						//x - (SrcT)min >= 0
						//Also, x - (SrcT)min == x - max - 1 < max
						dst_array[i] = (DstT)(x - (SrcT)min);
				}else{
					//Weird platform time!
					//TODO
					abort();
				}
			}
		}
	}
};

template <typename SrcT>
class SignednessFilterGeneric<TypeNil, SrcT> : public SignednessFilter{
public:
	SignednessFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format): SignednessFilter(src_format, dst_format){}
	void read(audio_buffer_t *buffers, size_t size){}
};

template <typename DstT, typename SrcT>
SignednessFilter *SignednessFilter::create_helper(const AudioFormat &src_format, const AudioFormat &dst_format){
	return new SignednessFilterGeneric<DstT, SrcT>(src_format, dst_format);
}

struct SignednessTest{
	template <typename T>
	bool operator()(const T &, const AudioFormat &af) const{
		return std::is_signed<T>::value == af.is_signed;
	}
};

SignednessFilter *SignednessFilter::create(const AudioFormat &src_format, const AudioFormat &dst_format){
	CreatorFunctor<SignednessFilter, SignednessTest> cf(dst_format, src_format);
	iterate_type_list(INTEGER_TYPE_LIST, cf);
	return cf.get_result();
}
