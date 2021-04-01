/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef MY_MPG123_H
#define MY_MPG123_H
#include <type_traits>

typedef std::make_signed<size_t>::type my_ssize_t;
#define ssize_t my_ssize_t
#include <mpg123.h>
#endif
