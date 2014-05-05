#include "AudioFilterPrivate.h"
#include "TypeList.h"
#include <SDL_stdinc.h>
#include <limits>
#include <cassert>
#include <boost/type_traits.hpp>

#define NODE(x) TypeCons<TypeNil, x>()
#define cons =

//Works between any two integer types.
template <typename DstT, typename SrcT>
class SignednessFilterGeneric : public SignednessFilter{
public:
	SignednessFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format): SignednessFilter(src_format, dst_format){}
	void read(audio_buffer_t &buffer){
		const SrcT *src_array = (const SrcT *)buffer.raw_pointer(0);
		const size_t array_size = buffer.byte_length() / sizeof(SrcT);
		DstT *dst_array = (DstT *)src_array;
		assert(boost::is_signed<DstT>::value != boost::is_signed<SrcT>::value);
		if (boost::is_signed<SrcT>::value){
			for (size_t i = 0; i != array_size; i++)
				dst_array[i] = (DstT)src_array[i] + (DstT)std::numeric_limits<SrcT>::min();
		}else{
			for (size_t i = 0; i != array_size; i++){
				SrcT x = src_array[i];
				const DstT min = std::numeric_limits<DstT>::min();
				const DstT max = std::numeric_limits<DstT>::max();
				const SrcT bigmax = std::numeric_limits<SrcT>::max();
				//This if should be eliminated at compile time.
				if (min == -max){
					//The easy case first:
					if (x <= max)
						dst_array[i] = (DstT)x + min;
					else
						dst_array[i] = (DstT)(x - (SrcT)min);
				}else if (min + 1 == -max){
					//two's complement case:
					//min + 1 == -max <=> max == -min - 1
					
					if (x <= max)
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
		buffer.set_sample_count(this->calculate_required_size(buffer.samples()));
	}
};



template <typename src_type>
SignednessFilter*create_sign_changed_step3(TypeNil, const AudioFormat &, const AudioFormat &){
	return 0;
}

template <typename src_type, typename T1, typename T2>
SignednessFilter *create_sign_changed_step3(TypeCons<T1, T2>, const AudioFormat &src_format, const AudioFormat &dst_format){
	if (sizeof(T2) == dst_format.bytes_per_channel)
		return new SignednessFilterGeneric<T2, src_type>(src_format, dst_format);
	return create_sign_changed_step3<src_type>(T1(), src_format, dst_format);
}


template <typename ListT>
SignednessFilter *create_sign_changed_step2(const TypeNil &, ListT &, const AudioFormat &, const AudioFormat &){
	return 0;
}

template <typename T1, typename T2, typename ListT>
SignednessFilter *create_sign_changed_step2(TypeCons<T1, T2>, const ListT &list, const AudioFormat &src_format, const AudioFormat &dst_format){
	if (sizeof(T2) == src_format.bytes_per_channel)
		return create_sign_changed_step3<T2>(list, src_format, dst_format);
	return create_sign_changed_step2(T1(), list, src_format, dst_format);
}

template <typename T1, typename T2>
SignednessFilter *create_sign_changed_step1(const TypeCons<T1, T2> &list, const AudioFormat &src_format, const AudioFormat &dst_format){
	return create_sign_changed_step2(list, list, src_format, dst_format);
}

SignednessFilter *SignednessFilter::create(const AudioFormat &src_format, const AudioFormat &dst_format){
	return create_sign_changed_step1(INTEGER_TYPE_LIST, src_format, dst_format);
}
