/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/
#include "stdafx.h"
#include "XorShift128.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <cstdint>
#include <algorithm>
#include <random>
#endif

xorshift128_state get_seed(){
	xorshift128_state ret;
	std::random_device rnd;
	for (auto &i : ret)
		i = rnd();
	return ret;
}

XorShift128::XorShift128(): state(get_seed()){}

std::uint32_t XorShift128::get(){
	auto x = state[3];
	x ^= x << 11;
	x ^= x >> 8;
	state[3] = state[2];
	state[2] = state[1];
	state[1] = state[0];
	x ^= state[0];
	x ^= state[0] >> 19;
	state[0] = x;
	return x;
}
