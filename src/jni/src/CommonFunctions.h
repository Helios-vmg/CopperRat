#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H
#include "BasicTypes.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <boost/type_traits.hpp>
#include <android/log.h>
#endif

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

template <typename T>
void parse_into_hms(std::basic_ostream<T> &stream, double total_seconds){
	if (total_seconds < 0){
		stream <<"??:??";
		return;
	}
	int seconds = (int)fmod(total_seconds, 60);
	total_seconds = (total_seconds - seconds) / 60.0;
	int minutes = (int)fmod(total_seconds, 60);
	total_seconds = (total_seconds - minutes) / 60.0;
	int hours = (int)total_seconds;
	if (hours > 0)
		stream <<std::setw(2)<<std::setfill((T)'0')<<hours<<":";
	stream <<std::setw(2)<<std::setfill((T)'0')<<minutes<<":"<<std::setw(2)<<std::setfill((T)'0')<<seconds;
}

template <typename T>
void utf8_to_string(std::basic_string<T> &dst, const unsigned char *buffer, size_t n){
	static const unsigned char utf8_lengths[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
		4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
	};
	static const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
	static const unsigned char masks[] = { 0x00, 0xFF, 0x1F, 0x0F, 0x07, 0x03, 0x01 };
	if (n >= 3 && !memcmp(buffer, bom, 3)){
		buffer += 3;
		n -= 3;
	}
	size_t truncated_length = 0,
		size = 0;
	for (truncated_length = 0; truncated_length < n;){
		auto l = utf8_lengths[buffer[truncated_length]];
		if (truncated_length + l > n)
			break;
		size++;
		truncated_length += l;
	}
	dst.resize(size);
	size_t writer = 0;
	T *pointer = &dst[0];
	for (size_t i = 0; i != truncated_length;){
		unsigned char byte = buffer[i++];
		if (i == 7563)
			byte = byte;
		auto length = utf8_lengths[byte];
		unsigned wc = byte & masks[length];
		while (--length){
			wc <<= 6;
			wc |= (T)(buffer[i++] & 0x3F);
		}
		pointer[writer++] = wc;
	}
}

inline unsigned utf8_bytes(unsigned c){
	if (c < 0x00000080)
		return 1;
	if (c < 0x00000800)
		return 2;
	if (c < 0x00010000)
		return 3;
	if (c < 0x00200000)
		return 4;
	if (c < 0x04000000)
		return 5;
	return 6;
}

template <typename T>
size_t utf8_size(const std::basic_string<T> &s){
	size_t ret = 0;
	for (auto c : s)
		ret += utf8_bytes(c);
	return ret;
}

template <typename T>
void string_to_utf8(std::vector<unsigned char> &dst, const std::basic_string<T> &src){
	static const unsigned char masks[] = { 0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
	dst.resize(utf8_size(src));
	unsigned char *pointer = &dst[0];
	const T *src_pointer = &src[0];
	size_t writer = 0;
	for (size_t i = 0, n = src.size(); i != n; i++){
		unsigned c = src_pointer[i];

		if (c < 0x80){
			*(pointer++) = (unsigned char)c;
			continue;
		}

		unsigned char temp[6];
		unsigned temp_size = 0;
		do{
			temp[temp_size++] = c & 0x3F | 0x80;
			c >>= 6;
		}while (c & ~0x3F);
		*(pointer++) = c | masks[temp_size];
		while (temp_size)
			*(pointer++) = temp[--temp_size];
	}
}

std::wstring utf8_to_string(const std::string &src);
std::string string_to_utf8(const std::wstring &src);

template <typename T1, typename T2>
bool check_flag(T1 flag, T2 pattern){
	return ((T2)flag & pattern) == pattern;
}

#ifndef __ANDROID__
#define __android_log_print(...)
#endif

bool read_32_bits(Uint32 &dst, std::istream &stream);
void read_32_big_bits(Uint32 &dst, const void *buf);
bool read_32_big_bits(Uint32 &dst, const std::vector<unsigned char> &src, size_t offset);

template <typename T, typename F>
bool glob(const T *pattern, const T *string, F f = F()){
glob_start:
	if (!*pattern && !*string)
		return 1;
	switch (*pattern){
		case '*':
			while (1){
				if (glob(pattern + 1, string, f))
					return 1;
				if (!*string)
					return 0;
				string++;
			}
		case '?':
			pattern++;
			string++;
			goto glob_start;
		default:
			if (!f && *pattern == *string || f && f(*pattern) == f(*string)){
				pattern++;
				string++;
				goto glob_start;
			}
			return 0;
	}
}

template <typename T>
std::basic_string<T> get_filename(const std::basic_string<T> &path){
	static const char slashes[] = { '/', '\\' };
	for (T c : slashes){
		auto slash = path.rfind(c);
		if (slash != path.npos)
			return path.substr(slash + 1);
	}
	return path;
}

template <typename T>
std::basic_string<T> get_contaning_directory(const std::basic_string<T> &path){
	static const char slashes[] = { '/', '\\' };
	for (T c : slashes){
		auto slash = path.rfind(c);
		if (slash != path.npos)
			return path.substr(0, slash + 1);
	}
	return std::basic_string<T>();
}

template <typename T>
inline T max_possible_value(T x){
	return std::numeric_limits<T>::max();
}

double get_dots_per_millimeter();

inline std::wstring to_wstring(const std::wstring &s){
	return s;
}

template <typename T>
std::wstring to_wstring(const std::basic_string<T> &s){
	std::wstring ret;
	ret.resize(s.size());
	typedef typename boost::make_unsigned<T>::type UT;
	for (auto i = s.size(); i--;)
		ret[i] = (wchar_t)(UT)s[i];
	return ret;
}

template <typename T>
std::basic_string<T> tolower(const std::basic_string<T> &s){
	auto ret = s;
	for (auto &c : ret)
		c = ::tolower(c);
	return ret;
}

template <typename T>
int strcmp_case(const std::basic_string<T> &a, const std::basic_string<T> &b){
	const T * const ap = &a[0];
	const T * const bp = &b[0];
	const auto n = std::min(a.size(), b.size());
	for (size_t i = 0; i < n; i++){
		int d = tolower(ap[i]) - tolower(bp[i]);
		if (d)
			return d;
	}
	return (int)a.size() - (int)b.size();
}

template <typename T>
std::basic_string<T> get_extension(const std::basic_string<T> &s){
	auto ext = s;
	auto dot = ext.rfind('.');
	if (dot == ext.npos)
		return L"";
	ext = ext.substr(dot + 1);
	return tolower(ext);
}

template <typename T>
bool path_is_rooted(const std::basic_string<T> &s){
	if (!s.size())
		return 0;
	return s[0] == '/' || s[0] == '\\';
}

#endif
