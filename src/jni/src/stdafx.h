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

#ifdef HAVE_PRECOMPILED_HEADERS
#if defined __ANDROID__
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <dirent.h>
#include <unistd.h>
#include <android/log.h>
#include <jni.h>
#elif defined WIN32
#include <Windows.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#else
#error Platform not supported!
#endif

#include <FLAC++/decoder.h>
#include <SDL.h>
#include <SDL_atomic.h>
#include <SDL_image.h>
#include <SDL_stdinc.h>
#include <SDL_system.h>
extern "C"{
#include <SDL_gpu.h>
}
#include <algorithm>
#include <boost/coroutine/all.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/type_traits.hpp>
#include <boost/rational.hpp>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <ogg/ogg.h>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <vorbis/vorbisfile.h>
#include <webp/encode.h>
#endif
