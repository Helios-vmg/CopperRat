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

#ifndef TYPELIST_H
#define TYPELIST_H

struct TypeNil{
	template <typename OriginalType, typename F>
	void iterate(F &f) const{}
};

template <int N>
struct TypeIntNode{
	static const int value = N;
};

template <int N>
struct TypeUnsignedNode{
	static const unsigned value = N;
};

template <typename T1, typename T2>
struct TypeCons;

template <typename T1, typename T2>
struct TypeAppend{
};

template <typename T1, typename T2, typename T3>
struct TypeAppend<TypeCons<T1, T2>, T3>{
	typedef TypeCons<typename TypeAppend<T1, T3>::type, T2> type;
};

template <typename T3>
struct TypeAppend<TypeNil, T3>{
	typedef TypeCons<TypeNil, T3> type;
	template <typename OriginalType, typename F>
	void iterate(F &) const{}
};

template <typename T1, typename T2>
struct TypeCons{
	template <typename T3>
	typename TypeAppend<TypeCons<T1, T2>, T3>::type operator+(const TypeCons<TypeNil, T3> &) const{
		return typename TypeAppend<TypeCons<T1, T2>, T3>::type();
	}
	template <typename OriginalType, typename F>
	void iterate(F &f) const{
		if (f(T2(), OriginalType()))
			T1().iterate<OriginalType>(f);
	}
};

template <typename ListType, typename F>
void iterate_type_list(const ListType &lt, F &f){
	lt.template iterate<ListType>(f);
}

#endif
