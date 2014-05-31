#ifndef IMAGE_H
#define IMAGE_H

#include <SDL.h>
#include <string>

SDL_Surface *scale_surface(SDL_Surface *src, unsigned dst_w, unsigned dst_h);
SDL_Surface *bind_surface_to_square(SDL_Surface *src, unsigned size);

SDL_Surface *load_image_from_file(const char *path);
SDL_Surface *load_image_from_file(const std::wstring &path);
SDL_Surface *load_image_from_memory(const void *buffer, size_t length);
void save_surface_compressed(const char *path, SDL_Surface *src);
#endif
