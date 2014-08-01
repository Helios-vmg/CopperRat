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

#ifndef IMAGE_H
#define IMAGE_H

#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#include <SDL.h>
#include <boost/shared_ptr.hpp>
#endif
#include "Deleters.h"

typedef boost::shared_ptr<SDL_Surface> surface_t;
typedef boost::shared_ptr<GPU_Renderer> renderer_t;
typedef boost::shared_ptr<GPU_Image> texture_t;
	typedef boost::shared_ptr<GPU_Target> render_target_t;

class RenderTarget{
	texture_t texture;
	render_target_t target;
public:
	RenderTarget(unsigned w, unsigned h);
	//RenderTarget(surface_t);
	GPU_Target *get_target(){
		return this->target.get();
	}
	texture_t get_image();
};

inline surface_t to_surface_t(SDL_Surface *s){
	return surface_t(s, SDL_Surface_deleter());
}

inline texture_t to_texture_t(GPU_Image *s){
	return texture_t(s, GPU_Image_deleter());
}

struct SurfaceLocker{
	surface_t &s;
	SurfaceLocker(surface_t &s): s(s){
		SDL_LockSurface(this->s.get());
	}
	~SurfaceLocker(){
		SDL_UnlockSurface(this->s.get());
	}
};

surface_t create_rgbq_surface(unsigned bits, unsigned w, unsigned h);
surface_t create_rgb_surface(unsigned w, unsigned h);
surface_t create_rgba_surface(unsigned w, unsigned h);
surface_t create_surface_without_copy(surface_t);
surface_t copy_surface(surface_t);

surface_t scale_surface(surface_t src, unsigned dst_w, unsigned dst_h);
surface_t bind_surface_to_square(surface_t src, unsigned size);

surface_t load_image_from_file(const char *path);
surface_t load_image_from_file(const std::wstring &path);
surface_t load_image_from_memory(const void *buffer, size_t length);
void save_surface_compressed(const char *path, surface_t src);

surface_t apply_gaussian_blur(surface_t, double sigma);
surface_t apply_gaussian_blur_double(surface_t, double sigma);
surface_t apply_gaussian_blur2(surface_t src_surface, double sigma);
surface_t apply_box_blur(surface_t src_surface, double radius);

class Texture{
	bool loaded;
	GPU_Target *target;
	texture_t tex;
	GPU_Rect rect;

	void from_surface(surface_t src);
public:
	Texture(): loaded(0){}
	Texture(GPU_Target *target): target(target), loaded(0){}
	Texture(GPU_Target *target, const std::wstring &path);
	Texture(GPU_Target *target, surface_t src);
	void set_target(GPU_Target *target){
		this->target = target;
		//this->tex.reset();
		//this->loaded = 0;
	}
	operator bool() const{
		return this->loaded;
	}
	const GPU_Rect &get_rect() const{
		return this->rect;
	}
	void draw(const SDL_Rect &dst, const SDL_Rect *src = nullptr);
	void draw_with_fill(GPU_Target *);
	void load(const std::wstring &path){
		this->from_surface(load_image_from_file(path));
	}
	void load(surface_t src){
		this->from_surface(src);
	}
	void operator=(texture_t tex){
		this->tex = tex;
		this->rect.x = 0;
		this->rect.y = 0;
		this->rect.w = tex->w;
		this->rect.h = tex->h;
		this->loaded = 1;
	}
	void unload(){
		this->tex.reset();
		this->loaded = 0;
	}
	void set_alpha(double alpha);
};

class Subtexture{
	Texture texture;
	SDL_Rect region;
	double scale;
public:
	Subtexture(){}
	Subtexture(Texture texture, const SDL_Rect &region, double scale = 1): texture(texture), region(region), scale(scale){}
	void draw(const SDL_Rect &dst);
	SDL_Rect get_rect() const{
		auto ret = this->region;
		ret.w = (int)(ret.w * this->scale);
		ret.h = (int)(ret.h * this->scale);
		return ret;
	}
};

#endif
