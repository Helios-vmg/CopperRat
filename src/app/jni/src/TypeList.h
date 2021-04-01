/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

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
			T1().template iterate<OriginalType>(f);
	}
};

template <typename ListType, typename F>
void iterate_type_list(const ListType &lt, F &f){
	lt.template iterate<ListType>(f);
}
