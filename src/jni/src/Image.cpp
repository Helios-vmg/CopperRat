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

#include "stdafx.h"
#include "Image.h"
#include "CommonFunctions.h"
#include "Deleters.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_image.h>
#include <webp/encode.h>
#include <fstream>
#include <cmath>
#include <boost/shared_array.hpp>
#endif

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
		byte_t(src->format->Rshift / 8),
		byte_t(src->format->Gshift / 8),
		byte_t(src->format->Bshift / 8),
		byte_t(src->format->Ashift / 8),
	};
	byte_t dst_offsets[] = {
		byte_t(dst->format->Rshift / 8),
		byte_t(dst->format->Gshift / 8),
		byte_t(dst->format->Bshift / 8),
		byte_t(dst->format->Ashift / 8),
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
	auto temp = to_surface_t(IMG_Load(path));
	if (!temp)
		return temp;
	return normalize_surface(temp);
}

surface_t load_image_from_file(const std::wstring &path){
	return load_image_from_file(string_to_utf8(path).c_str());
}

surface_t load_image_from_memory(const void *buffer, size_t length){
	return normalize_surface(to_surface_t(IMG_Load_RW(SDL_RWFromConstMem(buffer, (int)length), 1)));
}

void save_surface_compressed(const char *path, surface_t src, int quality){
	uint8_t *buffer;
	size_t buffer_size;
	if (src->format->BytesPerPixel == 3){
		SurfaceLocker sl(src);
		buffer_size = WebPEncodeRGB((const uint8_t *)src->pixels, src->w, src->h, src->pitch, quality, &buffer);
	}else{
		SurfaceLocker sl(src);
		buffer_size = WebPEncodeRGBA((const uint8_t *)src->pixels, src->w, src->h, src->pitch, quality, &buffer);
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

surface_t create_surface_without_copy(surface_t src){
	return to_surface_t(SDL_CreateRGBSurface(
		0,
		src->w,
		src->h,
		src->format->BitsPerPixel,
		src->format->Rmask,
		src->format->Gmask,
		src->format->Bmask,
		src->format->Amask));
}

surface_t copy_surface(surface_t s){
	surface_t ret = create_surface_without_copy(s);
	{
		SurfaceLocker sl0(s);
		SurfaceLocker sl1(ret);
		memcpy(ret->pixels, s->pixels, s->pitch * s->h);
	}
	return ret;
}

inline double gauss_kernel(unsigned x, unsigned y, double sigma){
	double s2 = sigma * sigma;
	return exp((x * x + y * y) / -(2 * s2)) / (2.0 * M_PI * s2);
}

inline double round(double x){
	return floor(x + 0.5);
}

inline double round_neg(double x){
	if (x > 0)
		return round(x);
	return ceil(x - 0.5);
}

double compute_gauss_normal(double sigma){
	double ret = 0;
	auto matrix_side = (unsigned)ceil(sigma * 3);

	for (unsigned x = 0; x < matrix_side; x++){
		for (unsigned y = 0; y < matrix_side; y++){
			double m;
			if (x && !y || !x && y)
				m = 2;
			else if (x && y)
				m = 4;
			else
				m = 1;
			ret += gauss_kernel(x, y, sigma) * m;
		}
	}
	return ret;
}

surface_t apply_gaussian_blur(surface_t src_surface, double sigma){
	sigma = abs(sigma);

	surface_t dst_surface = create_surface_without_copy(src_surface);
	{
		SurfaceLocker src_locker(src_surface);
		SurfaceLocker dst_locker(dst_surface);

		auto src_pixels = (unsigned char *)src_surface->pixels;
		auto pitch = src_surface->pitch;
		auto advance = src_surface->format->BytesPerPixel;
	
		auto dst_pixels = (unsigned char *)dst_surface->pixels;

		int w = src_surface->w;
		int h = src_surface->h;

		auto matrix_side = (unsigned)ceil(sigma * 3);
		boost::shared_array<unsigned> matrix(new unsigned[matrix_side * matrix_side]);
		unsigned *matrix_p = matrix.get();

		const unsigned nk = 12;
		const double k = 1 << nk;

		for (unsigned x = 0; x < matrix_side; x++){
			for (unsigned y = 0; y < matrix_side; y++){
				matrix_p[x + y * matrix_side] = (unsigned)round(k * gauss_kernel(x, y, sigma));
			}
		}

		auto normal = (unsigned)round(k * compute_gauss_normal(sigma));

		int max_d = matrix_side;

		for (int dst_y = 0; dst_y < h; dst_y++){
			int src_min_y = std::max(dst_y - max_d + 1, 0);
			int src_max_y = std::min(dst_y + max_d, h);
			for (int dst_x = 0; dst_x < w; dst_x++){
				unsigned accum[4] = {0};
				int src_min_x = std::max(dst_x - max_d + 1, 0);
				int src_max_x = std::min(dst_x + max_d, w);
				auto dst_pixel = dst_pixels + pitch * dst_y + advance * dst_x;
				unsigned normalization = 0;
				for (int src_y = src_min_y; src_y < src_max_y; src_y++){
					int dy = src_y - dst_y;
					auto mask = dy >> 31;
					dy = (dy + mask) ^ mask;
					for (int src_x = src_min_x; src_x < src_max_x; src_x++){
						int dx = src_x - dst_x;
						mask = dx >> 31;
						dx = (dx + mask) ^ mask;

						auto src_pixel = src_pixels + pitch * src_y + advance * src_x;
						auto factor = matrix_p[dx + dy * matrix_side];
						normalization += factor;
						accum[0] += src_pixel[0] * factor;
						accum[1] += src_pixel[1] * factor;
						accum[2] += src_pixel[2] * factor;
						if (advance != 4)
							continue;
						accum[3] += src_pixel[3] * factor;
					}
				}
				for (int i = 0; i < advance; i++)
					dst_pixel[i] = (unsigned char)((accum[i] * normal / normalization) >> nk);
			}
		}
	}
	return dst_surface;
}

surface_t apply_box_blur(surface_t src_surface, double radius){
	auto t0 = clock();
	radius = abs(radius);

	auto radius2 = (unsigned)ceil(radius * radius);

	surface_t dst_surface = create_surface_without_copy(src_surface);
	{
		SurfaceLocker src_locker(src_surface);
		SurfaceLocker dst_locker(dst_surface);

		auto src_pixels = (unsigned char *)src_surface->pixels;
		auto pitch = src_surface->pitch;
		auto advance = src_surface->format->BytesPerPixel;
	
		auto dst_pixels = (unsigned char *)dst_surface->pixels;

		int w = src_surface->w;
		int h = src_surface->h;

		const unsigned nk = 12;
		const double k = 1 << nk;

		int max_d = (int)ceil(radius);

		for (int dst_y = 0; dst_y < h; dst_y++){
			int src_min_y = std::max(dst_y - max_d + 1, 0);
			int src_max_y = std::min(dst_y + max_d, h);
			for (int dst_x = 0; dst_x < w; dst_x++){
				unsigned accum[4] = {0};
				int src_min_x = std::max(dst_x - max_d + 1, 0);
				int src_max_x = std::min(dst_x + max_d, w);
				auto dst_pixel = dst_pixels + pitch * dst_y + advance * dst_x;
				unsigned normalization = 0;
				for (int src_y = src_min_y; src_y < src_max_y; src_y++){
					int dy = src_y - dst_y;
					auto mask = dy >> 31;
					dy = (dy + mask) ^ mask;
					for (int src_x = src_min_x; src_x < src_max_x; src_x++){
						int dx = src_x - dst_x;
						mask = dx >> 31;
						dx = (dx + mask) ^ mask;

						if (unsigned(dx * dx + dy * dy) > radius2)
							continue;

						auto src_pixel = src_pixels + pitch * src_y + advance * src_x;
						normalization++;
						accum[0] += src_pixel[0];
						accum[1] += src_pixel[1];
						accum[2] += src_pixel[2];
						if (advance != 4)
							continue;
						accum[3] += src_pixel[3];
					}
				}
				for (int i = 0; i < advance; i++)
					dst_pixel[i] = (unsigned char)(accum[i] / normalization);
			}
		}
	}
	auto t1 = clock();
	std::cout <<t1 - t0<<std::endl;
	return dst_surface;
}

surface_t apply_gaussian_blur_double(surface_t src_surface, double sigma){
	sigma = abs(sigma);

	surface_t dst_surface = create_surface_without_copy(src_surface);
	{
		SurfaceLocker src_locker(src_surface);
		SurfaceLocker dst_locker(dst_surface);

		auto src_pixels = (unsigned char *)src_surface->pixels;
		auto pitch = src_surface->pitch;
		auto advance = src_surface->format->BytesPerPixel;
	
		auto dst_pixels = (unsigned char *)dst_surface->pixels;

		int w = src_surface->w;
		int h = src_surface->h;

		auto matrix_side = (unsigned)ceil(sigma * 3);
		boost::shared_array<double> matrix(new double[matrix_side * matrix_side]);
		double *matrix_p = matrix.get();

		for (unsigned x = 0; x < matrix_side; x++){
			for (unsigned y = 0; y < matrix_side; y++){
				matrix_p[x + y * matrix_side] = gauss_kernel(x, y, sigma);
			}
		}

		auto normal = compute_gauss_normal(sigma);

		int max_d = matrix_side;

		for (int dst_y = 0; dst_y < h; dst_y++){
			int src_min_y = std::max(dst_y - max_d + 1, 0);
			int src_max_y = std::min(dst_y + max_d, h);
			for (int dst_x = 0; dst_x < w; dst_x++){
				double accum[4] = {0};
				int src_min_x = std::max(dst_x - max_d + 1, 0);
				int src_max_x = std::min(dst_x + max_d, w);
				auto dst_pixel = dst_pixels + pitch * dst_y + advance * dst_x;
				double normalization = 0;
				for (int src_y = src_min_y; src_y < src_max_y; src_y++){
					int dy = src_y - dst_y;
					if (dy < 0)
						dy = -dy;
					for (int src_x = src_min_x; src_x < src_max_x; src_x++){
						int dx = src_x - dst_x;
						if (dx < 0)
							dx = -dx;

						auto src_pixel = src_pixels + pitch * src_y + advance * src_x;
						auto factor = matrix_p[dx + dy * matrix_side];
						normalization += factor;
						accum[0] += src_pixel[0] * factor;
						accum[1] += src_pixel[1] * factor;
						accum[2] += src_pixel[2] * factor;
						if (advance != 4)
							continue;
						accum[3] += src_pixel[3] * factor;
					}
				}
				for (int i = 0; i < advance; i++)
					dst_pixel[i] = (unsigned char)round(accum[i] * normal / normalization);
			}
		}
	}
	return dst_surface;
}

void gauss_boxes(int sizes[], size_t n, double sigma){
	double w_ideal = sqrt((12 * sigma * sigma / n) + 1);
	int wl = (int)w_ideal;
	if (wl % 2 == 0)
		wl--;
	int wu = wl + 2;

	double m_ideal = (12 * sigma * sigma - n * wl * wl - 4 * n * wl - 3 * n) / (-4 * wl - 4);
	int m = (int)round_neg(m_ideal);

	for (size_t i = 0; i < n; i++)
		sizes[i] = i < m ? wl : wu;
}

class GaussSurface{
	surface_t surface;
	unsigned char *pixels;
	unsigned channel_offset;
public:
	unsigned channel_count, w, h, pitch, advance;
	GaussSurface(surface_t surface): surface(surface){
		this->pixels = (unsigned char *)surface->pixels;
		this->w = surface->w;
		this->h = surface->h;
		this->pitch = surface->pitch;
		this->channel_count = this->advance = surface->format->BytesPerPixel;
		this->channel_offset = 0;
	}
	void set_channel_offset(unsigned channel){
		this->channel_offset = channel;
	}
	unsigned char *operator[](unsigned i){
		unsigned x = i % this->w;
		unsigned y = i / this->w;
		return this->get_pixel(x, y);
	}
	unsigned get_index(unsigned x, unsigned y){
		return x + y * this->w;
	}
	unsigned char *get_pixel(unsigned x, unsigned y){
		return this->pixels + y * this->pitch + x * this->advance + this->channel_offset;
	}
	void swap_dimensions(){
		std::swap(this->w, this->h);
		std::swap(this->advance, this->pitch);
	}
};

surface_t apply_gaussian_blur2(surface_t src_surface, double sigma){
	auto t0 = clock();
	sigma = abs(sigma);

	surface_t src_surface2 = copy_surface(src_surface);
	surface_t dst_surface = create_surface_without_copy(src_surface);
	src_surface = src_surface2;
	{
		SurfaceLocker src_locker(src_surface);
		SurfaceLocker dst_locker(dst_surface);

		GaussSurface src_gsurface = src_surface;
		GaussSurface dst_gsurface = dst_surface;

		size_t byte_length = src_surface->pitch * src_surface->h;

		int boxes[3];
		gauss_boxes(boxes, 3, sigma);

		for (int ii = 0; ii < 3; ii++){
			int r = (boxes[ii] - 1) / 2;

			const unsigned iarr = unsigned((1<<12) / (r * 2 + 1));
			for (int jj = 0; jj < 2; jj++){
				for(unsigned i = 0; i < dst_gsurface.h; i++){
					for (unsigned channel = 0; channel < dst_gsurface.channel_count; channel++){
						src_gsurface.set_channel_offset(channel);
						dst_gsurface.set_channel_offset(channel);
						int ti = dst_gsurface.get_index(0, i),
							li = ti,
							ri = ti + dst_gsurface.get_index(r, 0);
						int fv = *src_gsurface[ti],
							lv = *src_gsurface[ti + dst_gsurface.get_index(dst_gsurface.w - 1, 0)],
							val = (r + 1) * fv;
						for (int j = 0; j < r; j++)
							val += *src_gsurface[ti + j];
						for (int j = 0; j <= r; j++){
							val += *src_gsurface[ri++] - fv;
							*dst_gsurface[ti++] = ((val * iarr) + (1<<11)) >> 12;
						}
						for (int j = r + 1; j < (int)dst_gsurface.w - r; j++){
							val += *src_gsurface[ri++] - *src_gsurface[li++];
							*dst_gsurface[ti++] = ((val * iarr) + (1<<11)) >> 12;
						}
						for (int j = dst_gsurface.w - r; j < (int)dst_gsurface.w; j++) {
							val += lv - *src_gsurface[li++];
							*dst_gsurface[ti++] = ((val * iarr) + (1<<11)) >> 12;
						}
					}
				}

				std::swap(src_gsurface, dst_gsurface);
				src_gsurface.swap_dimensions();
				dst_gsurface.swap_dimensions();
			}
		}
	}
	auto t1 = clock();
	std::cout <<t1 - t0<<std::endl;
	return src_surface;
}

Texture::Texture(GPU_Target *target, const std::wstring &path):
		target(target),
		tex(nullptr, GPU_Image_deleter()),
		loaded(0),
		rect(){
	this->load(path);
}

Texture::Texture(GPU_Target *target, surface_t src):
		target(target),
		tex(nullptr, GPU_Image_deleter()),
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
	this->rect.w = (float)src->w;
	this->rect.h = (float)src->h;
	this->tex.reset(GPU_CopyImageFromSurface(src.get()), GPU_Image_deleter());
	this->loaded = !!tex.get();
}

GPU_Rect to_GPU_Rect(const SDL_Rect &rect){
	GPU_Rect ret;
	ret.x = (float)rect.x;
	ret.y = (float)rect.y;
	ret.w = (float)rect.w;
	ret.h = (float)rect.h;
	return ret;
}

void Texture::draw(const SDL_Rect &dst, const SDL_Rect *_src){
	if (!*this)
		return;
	GPU_Rect src = !_src ? this->rect : to_GPU_Rect(*_src);
	GPU_Blit(this->tex.get(), &src, this->target, (float)dst.x, (float)dst.y);
}

void Subtexture::draw(const SDL_Rect &_dst){
	if (!this->texture)
		return;
	SDL_Rect dst = _dst;
	dst.w = int(this->region.w * this->scale);
	dst.h = int(this->region.h * this->scale);
	this->texture.draw(dst, &this->region);
}

void Texture::draw_with_fill(GPU_Target *target){
	auto src_rect = this->rect;
	auto dst_rect = target->clip_rect;

	auto scale = std::max(dst_rect.w / src_rect.w, dst_rect.h / src_rect.h);

	src_rect.w *= scale;
	src_rect.h *= scale;

	src_rect.x = (dst_rect.w - src_rect.w) / 2;
	src_rect.y = (dst_rect.h - src_rect.h) / 2;

	auto tex = this->tex.get();
	GPU_BlitScale(tex, &this->rect, target, src_rect.x, src_rect.y, scale, scale);
}

void Texture::draw_with_fill2(GPU_Target *target){
	auto src_rect = this->rect;
	auto dst_rect = target->clip_rect;

	auto scale = std::max(dst_rect.w / src_rect.w, dst_rect.h / src_rect.h);

	src_rect.w *= scale;
	src_rect.h *= scale;

	src_rect.x = (dst_rect.w - src_rect.w) / 2;
	src_rect.y = (dst_rect.h - src_rect.h) / 2;

	auto tex = this->tex.get();
	GPU_BlitScale(tex, &this->rect, target, src_rect.x, src_rect.y, scale, scale);
}

void Texture::set_alpha(double alpha){
	if (!*this)
		return;
	SDL_Color color = { 0xFF, 0xFF, 0xFF, 0xFF };
	color.a = Uint8(alpha * 255.0);
	GPU_SetColor(this->tex.get(), &color);
}

RenderTarget::RenderTarget(unsigned w, unsigned h){
	auto image = GPU_CreateImage(w, h, GPU_FORMAT_RGBA);
	auto target = GPU_LoadTarget(image);
	this->texture.reset(image, GPU_Image_deleter());
	this->target.reset(target, GPU_Target_deleter());
}

texture_t RenderTarget::get_image(){
	if (!this->target)
		return texture_t();
	if (!this->texture)
		return to_texture_t(GPU_CopyImageFromTarget(this->target.get()));
	return this->texture;
}

Shader::Shader(const char *source, bool fragment_shader){
	this->shader = GPU_CompileShader(fragment_shader ? GPU_FRAGMENT_SHADER : GPU_VERTEX_SHADER, source);
	if (!this->shader)
		this->error_string = GPU_GetShaderMessage();
}

Shader::~Shader(){
	if (!*this)
		return;
	GPU_FreeShader(this->shader);
}

ShaderProgram::~ShaderProgram(){
	if (!*this)
		return;
	GPU_FreeShaderProgram(this->program);
}

void ShaderProgram::create_internal_object(){
	if (*this)
		return;
	std::vector<Uint32> shaders;
	shaders.reserve(this->shaders.size());
	for (auto s : this->shaders)
		shaders.push_back(s->get_shader());
	this->program = GPU_LinkShadersEx(&shaders[0], shaders.size());
	if (!this->program)
		this->error_string = GPU_GetShaderMessage();
	else{
		auto uloc = GPU_GetUniformLocation(this->program, "tex");
		GPU_SetUniformi(uloc, 0);
	}
}

void ShaderProgram::activate(){
	if (!*this){
		this->create_internal_object();
		if (!*this)
			return;
	}
	GPU_ShaderBlock block = GPU_LoadShaderBlock(this->program, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "modelViewProjection");
	GPU_ActivateShaderProgram(this->program, &block);
}
