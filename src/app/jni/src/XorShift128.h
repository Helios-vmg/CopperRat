/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include <cstdint>
#include <algorithm>
#include <array>
#include <cassert>

typedef std::array<std::uint32_t, 4> xorshift128_state;

template <typename T>
class UniformGenerator{
public:
	typedef T output_type;
	virtual T get() = 0;
};

class XorShift128 : public UniformGenerator<std::uint32_t>{
protected:
	xorshift128_state state;
public:
	XorShift128();
	XorShift128(std::uint32_t seed){
		this->state[0] = seed;
		this->state[1] = seed;
		this->state[2] = seed;
		this->state[3] = seed;
	}
	XorShift128(const xorshift128_state &seed) : state(seed){}
	XorShift128(xorshift128_state &&seed) : state(std::move(seed)){}
	XorShift128(const XorShift128 &) = delete;
	XorShift128 &operator=(const XorShift128 &) = delete;
	XorShift128(XorShift128 &&) = delete;
	XorShift128 &operator=(XorShift128 &&) = delete;
	std::uint32_t get();
	const xorshift128_state &get_state() const{
		return this->state;
	}
};

template <typename Gen32>
class ConstructedUniformGenerator32 : public UniformGenerator<std::uint32_t>{
	Gen32 *gen;
public:
	ConstructedUniformGenerator32(Gen32 &gen): gen(&gen){}
	ConstructedUniformGenerator32(const ConstructedUniformGenerator32 &) = delete;
	ConstructedUniformGenerator32 &operator=(const ConstructedUniformGenerator32 &) = delete;
	ConstructedUniformGenerator32(ConstructedUniformGenerator32 &&) = delete;
	ConstructedUniformGenerator32 &operator=(ConstructedUniformGenerator32 &&) = delete;
	std::uint32_t get(){
		return this->gen->get();
	}
};

template <typename Gen32>
class ConstructedUniformGenerator64 : public UniformGenerator<std::uint64_t>{
	Gen32 *gen;
public:
	ConstructedUniformGenerator64(Gen32 &gen): gen(&gen){}
	ConstructedUniformGenerator64(const ConstructedUniformGenerator64 &) = delete;
	ConstructedUniformGenerator64 &operator=(const ConstructedUniformGenerator64 &) = delete;
	ConstructedUniformGenerator64(ConstructedUniformGenerator64 &&) = delete;
	ConstructedUniformGenerator64 &operator=(ConstructedUniformGenerator64 &&) = delete;
	std::uint64_t get(){
		std::uint64_t ret = this->gen->get();
		ret <<= 32;
		ret |= this->gen->get();
		return ret;
	}
};

template <typename T>
struct UniformGeneratorSelector{};

template <>
struct UniformGeneratorSelector<std::uint32_t>{
public:
	template <typename T>
	using type = ConstructedUniformGenerator32<T>;
};

template <>
struct UniformGeneratorSelector<std::uint64_t>{
public:
	template <typename T>
	using type = ConstructedUniformGenerator64<T>;
};

template <typename T>
T unbias(UniformGenerator<T> &rng, T n){
	constexpr auto cmax = std::numeric_limits<T>::max();
	T limit;
	if (n >= cmax / 2)
		limit = n;
	else{
		auto max = cmax / 2 + 1;
		limit = max - (max + 1) % n;
	}
	T ret;
	do
		ret = rng.get();
	while (ret > limit);
	return ret % n;
}

template <typename T>
T unbias(UniformGenerator<T> &rng, T begin, T end){
	return unbias(rng, end - begin) + begin;
}

template <typename It>
void special_random_shuffle(It begin, It in_end, It out_end, UniformGenerator<size_t> &rng){
	assert(out_end <= in_end);
	size_t n = out_end - begin;
	size_t m = in_end - begin;
	for (size_t i = 0; i < n; i++){
		auto j = unbias(rng, i, m);
		if (i != j)
			std::swap(begin[i], begin[j]);
	}
}

template <typename It>
void cr_random_shuffle(It begin, It end, UniformGenerator<size_t> &rng){
	return special_random_shuffle(begin, end, end, rng);
}
