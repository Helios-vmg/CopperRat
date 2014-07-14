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

#ifndef AUTO_PTR_H
#define AUTO_PTR_H

namespace CR{

struct Null{};

template <typename T, typename D = Null>
class auto_ptr{
	T *p;
	D d;
	auto_ptr(const auto_ptr<T> &){}
public:
	explicit auto_ptr(T *p = nullptr, D d = nullptr): p(p), d(d){}
	~auto_ptr(){
		this->reset();
	}
	void reset(T *p = nullptr){
		this->d(this->p);
		this->p = p;
	}
	T *get(){
		return this->p;
	}
	T &operator*(){
		return *this->p;
	}
	T *operator->(){
		return this->p;
	}
	const T *operator->() const{
		return this->p;
	}
	operator bool() const{
		return this->p != nullptr;
	}
};

template <typename T>
class auto_ptr<T, Null>{
	T *p;
	auto_ptr(const auto_ptr<T> &){}
public:
	explicit auto_ptr(T *p = nullptr): p(p){}
	~auto_ptr(){
		this->reset();
	}
	void reset(T *p = nullptr){
		delete this->p;
		this->p = p;
	}
	T *get(){
		return this->p;
	}
	T &operator*(){
		return *this->p;
	}
	T *operator->(){
		return this->p;
	}
	const T *operator->() const{
		return this->p;
	}
	operator bool() const{
		return this->p != nullptr;
	}
};

}

#if defined __ANDROID__
#define CR_UNIQUE_PTR(x) CR::auto_ptr<x>
#define CR_UNIQUE_PTR2(x, y) CR::auto_ptr<x, y>
#else
#define CR_UNIQUE_PTR(x) std::unique_ptr<x>
#define CR_UNIQUE_PTR2(x, y) std::unique_ptr<x, y>
#endif

#endif
