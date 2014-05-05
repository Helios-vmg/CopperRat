#include "AudioFilterPrivate.h"
#include "TypeList.h"

#define NODE(x) TypeCons<TypeNil, x>()
#define cons =

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
	void read(audio_buffer_t &buffer){
		const SrcT *src_array = (const SrcT *)buffer.raw_pointer(0);
		const size_t array_size = buffer.byte_length() / sizeof(SrcT);
		DstT *dst_array = (DstT *)src_array;
		if (sizeof(DstT) > sizeof(SrcT)){
			for (size_t i = array_size; i--;)
				dst_array[i] = Converter<DstT, SrcT>::convert(src_array[i]);
		}else if (sizeof(DstT) < sizeof(SrcT)){
			for (size_t i = 0; i != array_size; i++)
				dst_array[i] = Converter<DstT, SrcT>::convert(src_array[i]);
		}
		buffer.set_sample_count(this->calculate_required_size(buffer.samples()));
	}
};



template <typename src_type>
BitShiftingFilter *create_shifter_step3(TypeNil, const AudioFormat &, const AudioFormat &){
	return 0;
}

template <typename src_type, typename T1, typename T2>
BitShiftingFilter *create_shifter_step3(TypeCons<T1, T2>, const AudioFormat &src_format, const AudioFormat &dst_format){
	if (sizeof(T2) == dst_format.bytes_per_channel)
		return new BitShiftingFilterGeneric<T2, src_type>(src_format, dst_format);
	return create_shifter_step3<src_type>(T1(), src_format, dst_format);
}


template <typename ListT>
BitShiftingFilter *create_shifter_step2(const TypeNil &, ListT &, const AudioFormat &, const AudioFormat &){
	return 0;
}

template <typename T1, typename T2, typename ListT>
BitShiftingFilter *create_shifter_step2(TypeCons<T1, T2>, const ListT &list, const AudioFormat &src_format, const AudioFormat &dst_format){
	if (sizeof(T2) == src_format.bytes_per_channel)
		return create_shifter_step3<T2>(list, src_format, dst_format);
	return create_shifter_step2(T1(), list, src_format, dst_format);
}

template <typename T1, typename T2>
BitShiftingFilter *create_shifter_step1(const TypeCons<T1, T2> &list, const AudioFormat &src_format, const AudioFormat &dst_format){
	return create_shifter_step2(list, list, src_format, dst_format);
}

BitShiftingFilter *BitShiftingFilter::create(const AudioFormat &src_format, const AudioFormat &dst_format){
	return create_shifter_step1(INTEGER_TYPE_LIST, src_format, dst_format);
}
