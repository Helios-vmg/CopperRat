#ifndef BASE64_H
#define BASE64_H
#include <vector>

void b64_decode(std::vector<unsigned char> &dst, const char *src, int slen);

#endif
