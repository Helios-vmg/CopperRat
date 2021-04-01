/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#include <vector>
#include <SDL.h>
#include <memory>
#endif
#include "Deleters.h"

typedef std::shared_ptr<SDL_Surface> surface_t;
typedef std::shared_ptr<GPU_Renderer> renderer_t;
typedef std::shared_ptr<GPU_Image> texture_t;
typedef std::shared_ptr<GPU_Target> render_target_t;

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
void save_surface_compressed(const char *path, surface_t src, int quality = 75);

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
	void draw_with_fill2(GPU_Target *);
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

class Shader;
typedef std::shared_ptr<Shader> shader_t;

class Shader{
	Uint32 shader;
	std::string error_string;
public:
	Shader(const char *source, bool fragment_shader = 1);
	~Shader();
	operator bool(){
		return !!this->shader;
	}
	Uint32 get_shader() const{
		return this->shader;
	}
	const std::string &get_error_string() const{
		return this->error_string;
	}
	static shader_t create(const char *source, bool fragment_shader = 1){
		return shader_t(new Shader(source, fragment_shader));
	}
};

class ShaderProgram{
	Uint32 program;
	std::vector<shader_t> shaders;
	std::string error_string;
public:
	ShaderProgram(): program(0) {}
	~ShaderProgram();
	operator bool(){
		return !!this->program;
	}
	void create_internal_object();
	void add(shader_t shader){
		if (!*shader)
			return;
		this->shaders.push_back(shader);
	}
	void activate();
	const std::string &get_error_string() const{
		return this->error_string;
	}
};
