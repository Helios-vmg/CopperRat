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

#ifndef DELETERS_H
#define DELETERS_H
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL.h>
#endif

struct SDL_Window_deleter{
	void operator()(SDL_Window *w) const{
		if (w)
			SDL_DestroyWindow(w);
	}
};

inline void SDL_Renderer_deleter_func(SDL_Renderer *r){
	if (r)
		SDL_DestroyRenderer(r);
}

struct SDL_Renderer_deleter{
	void operator()(SDL_Renderer *r) const{
		SDL_Renderer_deleter_func(r);
	}
};

inline void SDL_Surface_deleter_func(SDL_Surface *s){
	if (s)
		SDL_FreeSurface(s);
}

struct SDL_Surface_deleter{
	void operator()(SDL_Surface *s) const{
		SDL_Surface_deleter_func(s);
	}
};

inline void SDL_Texture_deleter_func(SDL_Texture *t){
	if (t)
		SDL_DestroyTexture(t);
}

struct SDL_Texture_deleter{
	void operator()(SDL_Texture *r) const{
		SDL_Texture_deleter_func(r);
	}
};

#endif
