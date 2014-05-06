#include "AudioFilterPrivate.h"
#include "TypeList.h"

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
	}
};

template <typename SrcT>
class BitShiftingFilterGeneric<TypeNil, SrcT> : public BitShiftingFilter{
public:
	BitShiftingFilterGeneric(const AudioFormat &src_format, const AudioFormat &dst_format): BitShiftingFilter(src_format, dst_format){}
	void read(audio_buffer_t &buffer){}
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
		return sizeof(T) == af.bytes_per_channel;
	}
};

BitShiftingFilter *BitShiftingFilter::create(const AudioFormat &src_format, const AudioFormat &dst_format){
	CreatorFunctor<BitShiftingFilter, BitShiftingTest> cf(dst_format, src_format);
	iterate_type_list(INTEGER_TYPE_LIST, cf);
	return cf.get_result();
}
