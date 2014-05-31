#include "Image.h"
#include "CommonFunctions.h"
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
		unsigned fractional_advance){
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
			for (unsigned i = 3 ; i--; )
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
		unsigned fractional_advance){
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
					multiplier = fixed_floor(X0)+unit-X0;
				else
					multiplier = unit;
				for (unsigned i = 3; i--; )
					color[i] += pixel[src_offsets[i]] * multiplier;
				pixel += src_advance;
				x0 = fixed_floor(x0) + unit;
			}
			for (unsigned i = 3; i--; )
				dst[dst_offsets[i]] = byte_t(color[i]/fractional_advance);
			dst += dst_advance;
			X0 = X1;
			X1 += fractional_advance;
		}
		dst = dst0 + dst_pitch;
		src += src_pitch;
	}
}

void do_transform(bool y_axis, double scale, SDL_Surface *src, SDL_Surface *dst){
	auto f = linear_interpolation1;
	if (scale < 1)
		f = linear_interpolation2;
	byte_t src_offsets[] = {
		src->format->Rshift / 8,
		src->format->Gshift / 8,
		src->format->Bshift / 8,
	};
	byte_t dst_offsets[] = {
		dst->format->Rshift / 8,
		dst->format->Gshift / 8,
		dst->format->Bshift / 8,
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
		unit / scale
	);
}

SDL_Surface *scale_surface(SDL_Surface *src, unsigned dst_w, unsigned dst_h){
	unsigned w = src->w;
	unsigned h = src->h;
	double xscale = (double)dst_w / (double)w;
	double yscale = (double)dst_h / (double)h;

	auto width_transformed = SDL_CreateRGBSurface(0, dst_w, h, 24, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
	SDL_LockSurface(width_transformed);
		auto temp = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGB24, 0);
		SDL_LockSurface(temp);
			do_transform(0, xscale, temp, width_transformed);
		SDL_UnlockSurface(temp);
		SDL_FreeSurface(temp);



		auto ret = SDL_CreateRGBSurface(0, dst_w, dst_h, 24, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
		SDL_LockSurface(ret);
		do_transform(1, yscale, width_transformed, ret);
		SDL_UnlockSurface(ret);

	SDL_UnlockSurface(width_transformed);
	SDL_FreeSurface(width_transformed);
	return ret;
}

SDL_Surface *bind_surface_to_square(SDL_Surface *src, unsigned size){
	if (src->w > src->h)
		return scale_surface(src, size, src->h * size / src->w);
	return scale_surface(src, src->w * size / src->h, size);
}

SDL_Surface *normalize_surface(SDL_Surface *s){
	if (!s)
		return s;
	auto ret = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGB24, 0);
	SDL_FreeSurface(s);
	return ret;
}

SDL_Surface *load_image_from_file(const char *path){
	return normalize_surface(IMG_Load(path));
}

SDL_Surface *load_image_from_file(const std::wstring &path){
	return load_image_from_file(string_to_utf8(path).c_str());
}

SDL_Surface *load_image_from_memory(const void *buffer, size_t length){
	return normalize_surface(IMG_Load_RW(SDL_RWFromConstMem(buffer, length), 1));
}

void save_surface_compressed(const char *path, SDL_Surface *src){
	uint8_t *buffer;
	size_t buffer_size;
	{
		SDL_LockSurface(src);
		buffer_size = WebPEncodeRGB((const uint8_t *)src->pixels, src->w, src->h, src->pitch, 75, &buffer);
		SDL_UnlockSurface(src);
	}
	{
		std::ofstream file(path, std::ios::binary);
		file.write((const char *)buffer, buffer_size);
	}
	free(buffer);
}
