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

#include "stdafx.h"
#include "CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <iomanip>
#include <vector>

#ifdef __ANDROID__
#include <SDL_system.h>
#include <jni.h>
#endif
#endif

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
	static double ret = -1;
	if (ret >= 0)
		return ret;
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz = env->FindClass("org/copper/rat/CopperRat");
	jmethodID getScreenDensity = env->GetMethodID(clazz, "getScreenDensity", "()D");
	ret = env->CallDoubleMethod(activity, getScreenDensity);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
	return ret;
#endif
}

int get_screen_width(){
#ifndef __ANDROID__
	return 540;
#else
	static int ret = -1;
	if (ret >= 0)
		return ret;
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz = env->FindClass("org/copper/rat/CopperRat");
	jmethodID getScreenWidth = env->GetMethodID(clazz, "getScreenWidth", "()I");
	ret = env->CallIntMethod(activity, getScreenWidth);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
	return ret;
#endif
}

int get_screen_height(){
#ifndef __ANDROID__
	return 960;
#else
	static int ret = -1;
	if (ret >= 0)
		return ret;
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz = env->FindClass("org/copper/rat/CopperRat");
	jmethodID getScreenHeight = env->GetMethodID(clazz, "getScreenHeight", "()I");
	ret = env->CallIntMethod(activity, getScreenHeight);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
	return ret;
#endif
}

bool is_inside(int x, int y, const SDL_Rect &region){
	return x >= region.x && x < region.x + region.w && y >= region.y && y < region.y + region.h;
}

std::string wide_to_narrow(const std::wstring &s){
	std::string ret;
	ret.reserve(s.size());
	for (auto wc : s)
		ret.push_back(wc < 128 ? wc : '?');
	return ret;
}
