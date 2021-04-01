/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

inline unsigned gcd(unsigned a, unsigned b){
	while (b){
		unsigned c = b;
		b = a % b;
		a = c;
	}
	return a;
}

inline unsigned lcm(unsigned a, unsigned b){
	return a * b / gcd(a, b);
}

template <typename T>
class Rational{
	T n, d;

	void reduce(){
		auto g = gcd(this->n, this->d);
		this->n /= g;
		this->d /= g;
		if (this->d < 0){
			this->n = -this->n;
			this->d = -this->d;
		}
	}
public:
	Rational(T n = 0, T d = 1): n(n), d(d){
		this->reduce();
	}
	Rational operator+(const Rational &other) const{
		return Rational(this->n * other.d + other.n * this->d, this->d * other.d);
	}
	Rational operator+(const T &other) const{
		return Rational(this->n + other * this->d, this->d);
	}
	Rational operator-(const Rational &other) const{
		return Rational(this->n * other.d - other.n * this->d, this->d * other.d);
	}
	Rational operator-(const T &other) const{
		return Rational(this->n - other * this->d, this->d);
	}
	Rational operator-() const{
		return Rational(-this->n, this->d);
	}
	Rational operator*(const Rational &other) const{
		return Rational(this->n * other.n, this->d * other.d);
	}
	Rational operator*(const T &other) const{
		return Rational(this->n * other, this->d);
	}
	Rational operator/(const Rational &other) const{
		return Rational(this->n * other.d, this->d * other.n);
	}
	bool operator==(const Rational &other) const{
		return this->n == other.n && this->d == other.d;
	}
	bool operator!=(const Rational &other) const{
		return !(*this == other);
	}
	bool operator<(const Rational &other) const{
		return this->n * other.d < other.n * this->d;
	}
	bool operator>(const Rational &other) const{
		return this->n * other.d > other.n * this->d;
	}
	bool operator<=(const Rational &other) const{
		return this->n * other.d <= other.n * this->d;
	}
	bool operator>=(const Rational &other) const{
		return this->n * other.d >= other.n * this->d;
	}
	bool operator==(const T &other) const{
		return this->n == other && (this->d == 1 || !other);
	}
	bool operator!=(const T &other) const{
		return !(*this == other);
	}
	bool operator<(const T &other) const{
		return this->n < other * this->d;
	}
	bool operator>(const T &other) const{
		return this->n > other * this->d;
	}
	bool operator<=(const T &other) const{
		return this->n <= other * this->d;
	}
	bool operator>=(const T &other) const{
		return this->n >= other * this->d;
	}
	explicit operator bool() const{
		return !!this->n;
	}
	explicit operator T() const{
		return this->n / this->d;
	}
	T numerator() const{
		return this->n;
	}
	T denominator() const{
		return this->d;
	}
};
