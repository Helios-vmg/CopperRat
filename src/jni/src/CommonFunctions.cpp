#include "CommonFunctions.h"
#include <iomanip>
#include <vector>

bool read_32_bits(Uint32 &dst, std::istream &stream){
	Uint8 temp[4];
	stream.read((char *)temp, sizeof(temp));
	if (stream.gcount() < sizeof(temp))
		return 0;
	dst = 0;
	for (int i = 4; i--;){
		dst <<= 8;
		dst |= temp[i];
	}
	return 1;
}

void read_32_big_bits(Uint32 &dst, const void *buf){
	const unsigned char *p = (const unsigned char *)buf;
	dst = 0;
	for (int i = 4; i--; p++){
		dst <<= 8;
		dst |= *p;
	}
}

bool read_32_big_bits(Uint32 &dst, const std::vector<unsigned char> &src, size_t offset){
	if (src.size() - offset < 4)
		return 0;
	read_32_big_bits(dst, &src[offset]);
	return 1;
}

std::wstring utf8_to_string(const std::string &src){
	std::wstring ret;
	utf8_to_string(ret, (const unsigned char *)&src[0], src.size());
	return ret;
}

std::string string_to_utf8(const std::wstring &src){
	std::string ret;
	std::vector<unsigned char> temp;
	string_to_utf8(temp, src);
	std::copy(temp.begin(), temp.end(), std::back_inserter(ret));
	return ret;
}

double get_dots_per_millimeter(){
#ifndef __ANDROID__
	return 4;
#else
	return 9.2753623188405797101449275362319;
#if 0
	static double dpm = -1;
	if (dpm >= 0)
		return dpm;
	dpm = /*TODO*/;
#endif
#endif
}
