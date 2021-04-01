/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL.h>
extern "C"{
#include "SDL_gpu/SDL_gpu.h"
}
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

inline void GPU_Image_deleter_func(GPU_Image *t){
	if (t)
		GPU_FreeImage(t);
}

struct GPU_Image_deleter{
	void operator()(GPU_Image *r) const{
		GPU_Image_deleter_func(r);
	}
};

inline void GPU_Target_deleter_func(GPU_Target *t){
	if (t)
		GPU_FreeTarget(t);
}

struct GPU_Target_deleter{
	void operator()(GPU_Target *r) const{
		GPU_Target_deleter_func(r);
	}
};
