#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H
#include "BasicTypes.h"
#include <iostream>

inline double s16_to_double(Sint16 x){
	return (x<0) ? (x/32768.0) : (x/32767.0);
}

inline Sint16 double_to_s16(double x){
	return Sint16((x<0) ? (x * 32768.0) : (x * 32767.0));
}

template <typename T>
T gcd(T a, T b){
	while (b){
		T temp = b;
		b = a % b;
		a = temp;
	}
	return a;
}

template <typename T>
bool is_power_of_2(T n){
	while (!(n & 1))
		n >>= 1;
	return n == 1;
}

template <typename T>
T integer_log2(T n){
	T ret = 0;
	while (n > 1){
		ret++;
		n >>= 1;
	}
	return ret;
}

template <typename T>
struct Unpointify{
};

template <typename T>
struct Unpointify<T *>{
	typedef T t;
};
#define UNPOINTER(x) typename Unpointify<x>::t

void parse_into_hms(std::ostream &stream, double total_seconds);


template <typename T>
bool utf8_to_string(std::basic_string<T> &dst, unsigned char *buffer, size_t n){
	static const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
	if (n >= 3 && !memcmp(buffer, bom, 3)){
		buffer += 3;
		n -= 3;
	}
	size_t utf8_size = 0;
	for (size_t i = 0; i < n; i++)
		if (buffer[i] < 128 || (buffer[i] & 192) == 192)
			utf8_size++;
	std::basic_string<T> &ret = dst;
	ret.reserve(utf8_size);
	for (size_t i = 0; i != n;){
		unsigned char byte = buffer[i++];
		T wc = 0;
		if (!(byte & 0x80))
			wc = (T)byte;
		else if ((byte & 0xC0) == 0x80)
			return 0;
		else if ((byte & 0xE0) == 0xC0){
			if (n - i < 2)
				return 0;
			wc = (T)(byte & 0x1F);
			wc <<= 6;
			wc |= (T)(buffer[i++] & 0x3F);
		}else if ((byte & 0xF0) == 0xE0){
			if (n - i < 3)
				return 0;
			wc = (T)(byte & 0x0F);
			wc <<= 6;
			wc |= (T)(buffer[i++] & 0x3F);
			wc <<= 6;
			wc |= (T)(buffer[i++] & 0x3F);
		}else if ((byte & 0xF8) == 0xF0){
			if (n - i < 4)
				return 0;
			wc = (T)(byte & 0x07);
			wc <<= 6;
			wc |= (T)(buffer[i++] & 0x3F);
			wc <<= 6;
			wc |= (T)(buffer[i++] & 0x3F);
			wc <<= 6;
			wc |= (T)(buffer[i++] & 0x3F);
		}else
			return 0;
		ret.push_back(wc);
	}
	return 1;
}

template <typename T1, typename T2>
bool check_flag(T1 flag, T2 pattern){
	return ((T2)flag & pattern) == pattern;
}

#ifdef __ANDROID__
#include <android/log.h>
#else
#define __android_log_print(...)
#endif

bool read_32_bits(Uint32 &dst, std::istream &stream);

#endif
