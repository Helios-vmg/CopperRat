/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioFilterPrivate.h"
#include "CommonFunctions.h"
#include "Rational.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#endif

typedef Rational<std::uint32_t> Q;

Q double_to_rational(double number){
	const auto max = std::numeric_limits<unsigned>::max();
	Q ret = 0;
	double x = number;
	int power = 0;
	while (x >= 2){
		x /= 2;
		power++;
	}
	for (int i = 0; i < 16; i++, power--){
		bool bit = x >= 1;
		if (bit)
			x--;
		x *= 2;
		if (bit){

			Q q2 = 1;
			if (power > 0)
				q2 = Q(1 << power, 1);
			else if (power < 0)
				q2 = Q(1, 1 << -power);
			Q q3 = ret + q2;
			if (q3.numerator() > max || q3.denominator() > max)
				break;
			ret = q3;
		}
	}
	return ret;
}

MultiplicationFilter::MultiplicationFilter(const AudioFormat &format, double scaling_factor): AudioFilter(format, format){
	if (format.channels != 2 || format.bytes_per_channel != 2 || !format.is_signed)
		throw CR_Exception("MultiplicationFilter::MultiplicationFilter(): Unimplemented filter.");
	auto q = double_to_rational(scaling_factor);
	this->factor = scaling_factor;
	this->multiplier = q.numerator();
	this->divider = q.denominator();
}

template <typename T1, typename T2>
T1 saturate(T2 n){
	const auto max = std::numeric_limits<T1>::max();
	const auto min = std::numeric_limits<T1>::min();
	if (n > (T2)max)
		return max;
	if (n < (T2)min)
		return min;
	return n;
}

void MultiplicationFilter::read(audio_buffer_t *buffers, size_t size){
	audio_buffer_t &buffer = buffers[0];
	auto dst = buffer.get_sample<Sint16, 2>(0);
	const memory_sample_count_t samples = buffer.samples();
	for (memory_sample_count_t i = 0; i != samples; i++){
		auto src = *buffer.get_sample<Sint16, 2>(i);
		Sint32 v[2] = {
			src.values[0],
			src.values[1],
		};
		v[0] *= this->multiplier;
		v[1] *= this->multiplier;
		v[0] /= this->divider;
		v[1] /= this->divider;
		dst[i].values[0] = saturate<Sint16>(v[0]);
		dst[i].values[1] = saturate<Sint16>(v[1]);
	}
}
