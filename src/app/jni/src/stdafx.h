/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
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
#include <memory>
#include <functional>
#include <type_traits>
#include <string_view>
#include <random>
#include <array>
#include <thread>
#include "tremor/ivorbisfile.h"
//#include <vorbis/vorbisfile.h>
#include <webp/encode.h>
#else
namespace std { typedef decltype(nullptr) nullptr_t; }
#endif
