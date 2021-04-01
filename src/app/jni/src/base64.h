/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef BASE64_H
#define BASE64_H
#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#endif

void b64_decode(std::vector<unsigned char> &dst, const char *src, int slen);

#endif
