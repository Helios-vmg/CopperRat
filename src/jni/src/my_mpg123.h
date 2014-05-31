#ifndef MY_MPG123_H
#define MY_MPG123_H
#include <boost/type_traits.hpp>

typedef boost::make_signed<size_t>::type my_ssize_t;
#define ssize_t my_ssize_t
#include <mpg123.h>
#endif
