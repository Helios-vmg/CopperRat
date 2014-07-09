#ifndef EXCEPTION_H
#define EXCEPTION_H

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
};

#endif
