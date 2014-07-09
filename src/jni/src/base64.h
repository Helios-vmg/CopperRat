#ifndef BASE64_H
#define BASE64_H
#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#endif

void b64_decode(std::vector<unsigned char> &dst, const char *src, int slen);

#endif
