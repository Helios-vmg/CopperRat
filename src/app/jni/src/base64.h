/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#endif

void b64_decode(std::vector<unsigned char> &dst, const char *src, int slen);
