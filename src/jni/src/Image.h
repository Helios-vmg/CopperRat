#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <SDL.h>
#include <boost/shared_ptr.hpp>
#include "Deleters.h"

typedef boost::shared_ptr<SDL_Surface> surface_t;
typedef boost::shared_ptr<SDL_Renderer> renderer_t;
typedef boost::shared_ptr<SDL_Texture> texture_t;

inline surface_t to_surface_t(SDL_Surface *s){
	return surface_t(s, SDL_Surface_deleter());
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

surface_t create_rgb_surface(unsigned w, unsigned h);
surface_t create_rgba_surface(unsigned w, unsigned h);

surface_t scale_surface(surface_t src, unsigned dst_w, unsigned dst_h);
surface_t bind_surface_to_square(surface_t src, unsigned size);

surface_t load_image_from_file(const char *path);
surface_t load_image_from_file(const std::wstring &path);
surface_t load_image_from_memory(const void *buffer, size_t length);
void save_surface_compressed(const char *path, surface_t src);

class Texture{
	bool loaded;
	renderer_t renderer;
	texture_t tex;
	SDL_Rect rect;

	void from_surface(surface_t src);
public:
	Texture(): loaded(0){}
	Texture(renderer_t renderer): renderer(renderer), loaded(0){}
	Texture(renderer_t renderer, const std::wstring &path);
	Texture(renderer_t renderer, surface_t src);
	void set_renderer(renderer_t renderer){
		this->renderer = renderer;
		this->tex.reset();
		this->loaded = 0;
	}
	operator bool() const{
		return this->loaded;
	}
	const SDL_Rect &get_rect() const{
		return this->rect;
	}
	void draw(const SDL_Rect &dst, const SDL_Rect *src = nullptr);
	void load(const std::wstring &path){
		this->from_surface(load_image_from_file(path));
	}
	void load(surface_t src){
		this->from_surface(src);
	}
	void unload(){
		this->tex.reset();
		this->loaded = 0;
	}
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
