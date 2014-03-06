#ifndef __FASTMATH_H_
#define __FASTMATH_H_

#pragma once

inline float fast_log2 (float val) {
	int * const  exp_ptr = reinterpret_cast <int *> (&val);
	int          x = *exp_ptr;
	const int    log_2 = ((x >> 23) & 255) - 128;
	x &= ~(255 << 23);
	x += 127 << 23;
	*exp_ptr = x;

	return (val + log_2);
}

inline float fast_log (const float &val) {
	return (fast_log2 (val) * 0.69314718f);
}

inline float very_fast_exp(float x) {
	return 1
		-x*(0.9999999995f
		-x*(0.4999999206f

		-x*(0.1666653019f
		-x*(0.0416573475f
		-x*(0.0083013598f

		-x*(0.0013298820f
		-x*(0.0001413161f)))))));
}

inline float fast_exp(float x) {
	bool lessZero = true;
	if (x < 0) {
		lessZero = false;
		x = -x;
	}

	// This dirty little trick only works because of the normalization and the fact that one element in the normalization is 1
	if (x > 20)
		return 0;
	int mult = 0;

	while (x > 0.69*2*2*2) {
		mult+=3;
		x /= 8.0f;
	}

	while (x > 0.69*2*2) {
		mult+=2;
		x /= 4.0f;
	}
	while (x > 0.69) {
		mult++;
		x /= 2.0f;
	}

	x = very_fast_exp(x);
	while (mult) {
		mult--;
		x = x*x;
	}

	if (lessZero) {
		return 1 / x;

	} else {
		return x;
	}
}

#endif
