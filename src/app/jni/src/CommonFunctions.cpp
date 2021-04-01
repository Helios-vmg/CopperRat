/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
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
    auto env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    auto activity = (jobject)SDL_AndroidGetActivity();
    auto clazz = env->FindClass("org/copper/rat/CopperRat");
    auto getScreenDensity = env->GetMethodID(clazz, "getScreenDensity", "()D");
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
    auto env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    auto activity = (jobject)SDL_AndroidGetActivity();
    auto clazz = env->FindClass("org/copper/rat/CopperRat");
    auto getScreenWidth = env->GetMethodID(clazz, "getScreenWidth", "()I");
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
	auto env = (JNIEnv *)SDL_AndroidGetJNIEnv();
	auto activity = (jobject)SDL_AndroidGetActivity();
	auto clazz = env->FindClass("org/copper/rat/CopperRat");
	auto getScreenHeight = env->GetMethodID(clazz, "getScreenHeight", "()I");
	ret = env->CallIntMethod(activity, getScreenHeight);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
	return ret;
#endif
}

void initialize_resources(){
#ifdef __ANDROID__
	auto env = (JNIEnv *)SDL_AndroidGetJNIEnv();
	auto activity = (jobject)SDL_AndroidGetActivity();
	auto clazz = env->FindClass("org/copper/rat/CopperRat");
	auto initializeAppDirectory = env->GetMethodID(clazz, "initializeAppDirectory", "()V");
	env->CallVoidMethod(activity, initializeAppDirectory);
	env->DeleteLocalRef(clazz);
#endif
}

std::wstring get_external_storage_path(){
#ifndef __ANDROID__
	return {};
#else
	static std::wstring path;
	if (path.size())
		return path;
	auto env = (JNIEnv *)SDL_AndroidGetJNIEnv();
	auto activity = (jobject)SDL_AndroidGetActivity();
	auto clazz = env->FindClass("org/copper/rat/CopperRat");
	auto getExternalStoragePath = env->GetMethodID(clazz, "getExternalStoragePath", "()Ljava/lang/String;");
	auto s = (jstring)env->CallObjectMethod(activity, getExternalStoragePath);
	auto s2 = env->GetStringUTFChars(s, nullptr);
	std::wstring ret;
	utf8_to_string(ret, (const unsigned char *)s2, strlen(s2));
	env->ReleaseStringUTFChars(s, s2);
	env->DeleteLocalRef(s);
	env->DeleteLocalRef(clazz);
	env->DeleteLocalRef(activity);

	path = ret;
	return ret;
#endif
}

void *android_get_player(){
#ifndef __ANDROID__
    return nullptr;
#else
    auto env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    auto activity = (jobject)SDL_AndroidGetActivity();
    auto clazz = env->FindClass("org/copper/rat/CopperRat");
    auto getPlayer = env->GetMethodID(clazz, "getPlayer", "()J");
    auto ret = env->CallLongMethod(activity, getPlayer);
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
    return (void *)(intptr_t)ret;
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
