/*

Copyright (c) 2014, 2021, Helios
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
