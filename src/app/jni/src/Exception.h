/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#include <exception>
#endif

#ifdef _MSC_VER
#define NO_EXCEPT
#else
#define NO_EXCEPT noexcept(true)
#endif

class CR_Exception : public std::exception{
public:
	std::string description;
	CR_Exception(){}
	CR_Exception(const std::string &description): description(description){}
	virtual ~CR_Exception(){}
	const char *what() const NO_EXCEPT{
		return this->description.c_str();
	}
	virtual CR_Exception *clone() const{
		return new CR_Exception(*this);
	}
};
