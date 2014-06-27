#include "Image.h"
#include "CommonFunctions.h"
#include "Deleters.h"
#include <SDL_image.h>
#include <webp/encode.h>
#include <fstream>

typedef unsigned char byte_t;

const unsigned unit = 1 << 16;

#define GET_FRACTION(x) ((((x) & (unit - 1)) << 16) / unit)

inline unsigned get_fractional_part(unsigned x){
	return x & 0xFFFF;
}

inline unsigned to_integer(unsigned x){
	return x >> 16;
}

inline unsigned fixed_floor(unsigned x){
	return x & ~(unit - 1);
}

inline unsigned fixed_ceil(unsigned x){
	return fixed_floor(x + (unit - 1));
}

//Fast.
//Scales [1;+Inf.): works
//Scales (0;1): works with decreasing precision as the scale approaches 0
void linear_interpolation1(
		byte_t *dst,
		unsigned w,
		unsigned h,
		unsigned dst_advance,
		unsigned dst_pitch,
		const byte_t *src,
		unsigned src_w,
		unsigned src_h,
		unsigned src_advance,
		unsigned src_pitch,
		const byte_t *dst_offsets,
		const byte_t *src_offsets,
		unsigned fractional_advance,
		unsigned bytes_per_pixel){
	unsigned X = 0;
	const byte_t black[] = { 0, 0, 0 };
	for (unsigned x = 0; x < w; x++){
		byte_t *dst0 = dst;
		const byte_t *pixel[2];
		unsigned conditional_pitch;
		{
			unsigned p = to_integer(X);
			pixel[0] = src + p * src_advance;
			if (p < src_w - 1){
				pixel[1] = pixel[0] + src_advance;
				conditional_pitch = src_pitch;
			}else{
				pixel[1] = black;
				conditional_pitch = 0;
			}
		}
		unsigned weight[2];
		weight[1] = get_fractional_part(X);
		weight[0] = unit - weight[1];
		for (unsigned y = 0; y < h; y++){
			for (unsigned i = bytes_per_pixel; i--; )
				dst[dst_offsets[i]] = (byte_t)to_integer(pixel[0][src_offsets[i]] * weight[0] + pixel[1][src_offsets[i]] * weight[1]);
			pixel[0] += src_pitch;
			pixel[1] += conditional_pitch;
			dst += dst_pitch;
		}
		X += fractional_advance;
		dst = dst0 + dst_advance;
	}
}

//Slow.
//Scales [1;+Inf.): doesn't work
//Scales (0;1): works
void linear_interpolation2(
		byte_t *dst,
		unsigned w,
		unsigned h,
		unsigned dst_advance,
		unsigned dst_pitch,
		const byte_t *src,
		unsigned src_w,
		unsigned src_h,
		unsigned src_advance,
		unsigned src_pitch,
		const byte_t *dst_offsets,
		const byte_t *src_offsets,
		unsigned fractional_advance,
		unsigned bytes_per_pixel){
	for (unsigned y = 0; y < h; y++){
		byte_t *dst0 = dst;
		unsigned X0 = 0,
			X1 = fractional_advance;
		for (unsigned x = 0; x < w; x++){
			const byte_t *pixel = src + to_integer(X0) * src_advance;
			unsigned color[4] = {0};
			for (unsigned x0 = X0; x0 < X1;){
				unsigned multiplier;
				if (X1 - x0 < unit)
					multiplier = X1 - x0;
				else if (x0 == X0)
					multiplier = fixed_floor(X0) + unit - X0;
				else
					multiplier = unit;
				for (unsigned i = bytes_per_pixel; i--; )
					color[i] += pixel[src_offsets[i]] * multiplier;
				pixel += src_advance;
				x0 = fixed_floor(x0) + unit;
			}
			for (unsigned i = bytes_per_pixel; i--; )
				dst[dst_offsets[i]] = byte_t(color[i]/fractional_advance);
			dst += dst_advance;
			X0 = X1;
			X1 += fractional_advance;
		}
		dst = dst0 + dst_pitch;
		src += src_pitch;
	}
}

void do_transform(bool y_axis, double scale, surface_t src, surface_t dst){

	auto f = linear_interpolation1;
	if (scale < 1)
		f = linear_interpolation2;
	byte_t src_offsets[] = {
		src->format->Rshift / 8,
		src->format->Gshift / 8,
		src->format->Bshift / 8,
		src->format->Ashift / 8,
	};
	byte_t dst_offsets[] = {
		dst->format->Rshift / 8,
		dst->format->Gshift / 8,
		dst->format->Bshift / 8,
		dst->format->Ashift / 8,
	};
	unsigned src_w = src->w;
	unsigned src_h = src->h;
	unsigned src_advance = src->format->BytesPerPixel;
	unsigned src_pitch= src->pitch;
	unsigned dst_w = dst->w;
	unsigned dst_h = dst->h;
	unsigned dst_advance = dst->format->BytesPerPixel;
	unsigned dst_pitch= dst->pitch;
	if (y_axis){
		std::swap(src_h,     src_w);
		std::swap(src_pitch, src_advance);
		std::swap(dst_h,     dst_w);
		std::swap(dst_pitch, dst_advance);
	}
	f(
		(byte_t *)dst->pixels,
		dst_w, dst_h, dst_advance, dst_pitch,
		(const byte_t *)src->pixels, 
		src_w, src_h, src_advance, src_pitch,
		dst_offsets,
		src_offsets,
		(unsigned)(unit / scale),
		src->format->BytesPerPixel
	);
}

surface_t normalize_surface(surface_t s){
	const auto format_24 = SDL_PIXELFORMAT_RGB24;
	const auto format_32 = SDL_PIXELFORMAT_RGBA8888;
	const auto format = s->format->BitsPerPixel == 24 ? format_24 : format_32;
	return !s ? s : to_surface_t(SDL_ConvertSurfaceFormat(s.get(), format, 0));
}

surface_t scale_surface(surface_t src, unsigned dst_w, unsigned dst_h){
	unsigned w = src->w;
	unsigned h = src->h;
	double xscale = (double)dst_w / (double)w;
	double yscale = (double)dst_h / (double)h;

	auto width_transformed = create_rgbq_surface(src->format->BitsPerPixel, dst_w, h);
	SurfaceLocker sl0(width_transformed);


	{
		auto temp = normalize_surface(src);
		SurfaceLocker sl1(temp);
		do_transform(0, xscale, temp, width_transformed);
	}


	auto ret = create_rgbq_surface(src->format->BitsPerPixel, dst_w, dst_h);
	{
		SurfaceLocker sl1(ret);
		do_transform(1, yscale, width_transformed, ret);
	}
	return ret;
}

surface_t bind_surface_to_square(surface_t src, unsigned size){
	if (src->w > src->h)
		return scale_surface(src, size, src->h * size / src->w);
	return scale_surface(src, src->w * size / src->h, size);
}

surface_t load_image_from_file(const char *path){
	return normalize_surface(to_surface_t(IMG_Load(path)));
}

surface_t load_image_from_file(const std::wstring &path){
	return load_image_from_file(string_to_utf8(path).c_str());
}

surface_t load_image_from_memory(const void *buffer, size_t length){
	return normalize_surface(to_surface_t(IMG_Load_RW(SDL_RWFromConstMem(buffer, (int)length), 1)));
}

void save_surface_compressed(const char *path, surface_t src){
	uint8_t *buffer;
	size_t buffer_size;
	{
		SurfaceLocker sl(src);
		buffer_size = WebPEncodeRGB((const uint8_t *)src->pixels, src->w, src->h, src->pitch, 75, &buffer);
	}
	{
		std::ofstream file(path, std::ios::binary);
		file.write((const char *)buffer, buffer_size);
	}
	free(buffer);
}

surface_t create_rgbq_surface(unsigned bits, unsigned w, unsigned h){
	return to_surface_t(SDL_CreateRGBSurface(0, w, h, bits, 0xFF, 0xFF00, 0xFF0000, 0xFF000000));
}

surface_t create_rgb_surface(unsigned w, unsigned h){
	return create_rgbq_surface(24, w, h);
}

surface_t create_rgba_surface(unsigned w, unsigned h){
	return create_rgbq_surface(32, w, h);
}

Texture::Texture(boost::shared_ptr<SDL_Renderer> renderer, const std::wstring &path):
		renderer(renderer),
		tex(nullptr, SDL_Texture_deleter()),
		loaded(0),
		rect(){
	this->load(path);
}

Texture::Texture(boost::shared_ptr<SDL_Renderer> renderer, surface_t src):
		renderer(renderer),
		tex(nullptr, SDL_Texture_deleter()),
		loaded(0),
		rect(){
	this->load(src);
}

void Texture::from_surface(surface_t src){
	if (!src)
		return;
	this->loaded = 0;
	this->tex.reset();
	this->rect.x = 0;
	this->rect.y = 0;
	this->rect.w = src->w;
	this->rect.h = src->h;
	this->tex.reset(SDL_CreateTextureFromSurface(this->renderer.get(), src.get()), SDL_Texture_deleter());
	this->loaded = !!tex.get();
}

void Texture::draw(const SDL_Rect &_dst, const SDL_Rect *_src){
	if (!*this)
		return;
	SDL_Rect src = !_src ? this->rect : *_src;
	SDL_Rect dst = _dst;
	dst.w = src.w;
	dst.h = src.h;
	SDL_RenderCopy(this->renderer.get(), this->tex.get(), &src, &dst);
}

void Subtexture::draw(const SDL_Rect &_dst){
	if (!this->texture)
		return;
	SDL_Rect dst = _dst;
	dst.w = int(this->region.w * this->scale);
	dst.h = int(this->region.h * this->scale);
	this->texture.draw(dst, &this->region);
}
