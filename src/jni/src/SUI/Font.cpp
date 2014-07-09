#include "../stdafx.h"
#include "Font.h"
#include "../CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_image.h>
#include <SDL.h>
#include <algorithm>
#include <sstream>
#include <cassert>
#endif

void Font::initialize_width_bitmap(Uint32 bitmap_offset, Uint32 offsets_table_offset){
	this->font_file.seekg(bitmap_offset);
	std::vector<char> vector_string(offsets_table_offset - bitmap_offset);
	this->font_file.read((char *)&vector_string[0], vector_string.size());
	if ((unsigned)this->font_file.gcount() < vector_string.size())
		throw CR_Exception("Invalid font file. No width bitmap.");
	std::string s;
	std::copy(vector_string.begin(), vector_string.end(), std::back_inserter(s));
	std::stringstream sstream(s);
	this->width_bitmap.reserve(1 << 16);
	int state;
	sstream >>state;
	int times;
	while (sstream >>times){
		while (times--)
			this->width_bitmap.push_back(!!state);
		state = !state;
	}
	assert(this->width_bitmap.size() == 1<<16);
}

void Font::initialize_offsets_table(Uint32 bitmap_offset, Uint32 offsets_table_offset){
	this->font_file.seekg(offsets_table_offset);
	const unsigned n = 1 << 8;
	this->offsets_table.resize(n + 1);
	for (int i = 0; i < n; i++){
		if (!read_32_bits(this->offsets_table[i], this->font_file))
			throw CR_Exception("Invalid font file. Incomplete offsets table.");
	}
	this->font_file.seekg(0, std::ios::end);
	this->offsets_table.back() = (Uint32)this->font_file.tellg();
}

Font::Font(boost::shared_ptr<SDL_Renderer> renderer): renderer(renderer){
	static const char *path = BASE_PATH "unifont.dat";
	this->font_file.open(path, std::ios::binary);
	if (!this->font_file)
		throw CR_Exception(std::string("Couldn't open font file: \"") + path + "\".");
	Uint32 bitmap_offset,
		offsets_table_offset;
	
	if (!read_32_bits(bitmap_offset, this->font_file) || !read_32_bits(offsets_table_offset, this->font_file))
		throw CR_Exception("Invalid font file. No offsets.");

	this->initialize_width_bitmap(bitmap_offset, offsets_table_offset);

	this->initialize_offsets_table(bitmap_offset, offsets_table_offset);

	this->textures.resize(1 << 8);

	this->load_page(0);
}

void preprocess_image(SDL_Surface *img){
	SDL_LockSurface(img);
	auto pitch = img->pitch;
	auto bps = img->format->BytesPerPixel;
	auto w = img->w;
	auto h = img->h;
	auto pixels = (Uint8 *)img->pixels;
	Uint32 cmask = img->format->Rmask | img->format->Gmask | img->format->Bmask;
	Uint32 amask = img->format->Amask;
	Uint32 gmask = img->format->Gmask;
	for (int y = 0; y < h; y++){
		for (int x = 0; x < h; x++){
			auto pixel = (Uint32 *)(pixels + x * bps + y * pitch);
			if (*pixel & cmask)
				*pixel |= amask;
			else
				*pixel = 0;
		}
	}
	for (int cy = 0; cy < 16; cy++){
		for (int cx = 0; cx < 16; cx++){
			for (int y = 0; y < 16; y++){
				for (int x = 0; x < 16; x++){
					int x0 = cx * 16 + x,
						y0 = cy * 16 + y,
						w0 = 16,
						h0 = 16;
					bool black = 0;
					auto px = pixels + x0 * bps + y0 * pitch;
					auto pixel = (Uint32 *)px;
					auto pixel0 = (Uint32 *)(px - bps);
					auto pixel1 = (Uint32 *)(px + bps);
					auto pixel2 = (Uint32 *)(px - pitch);
					auto pixel3 = (Uint32 *)(px + pitch);
					if (!black && x > 0)
						black = *pixel0 == (amask | cmask);
					if (!black && x < w0 - 1)
						black = *pixel1 == (amask | cmask);
					if (!black && y > 0)
						black = *pixel2 == (amask | cmask);
					if (!black && y < h0 - 1)
						black = *pixel3 == (amask | cmask);
					if (black)
						*pixel |= amask;
				}
			}
		}
	}
	std::ofstream raw("test.raw", std::ios::binary);
	raw.write((const char *)pixels, 256 * 256 * 4);
	raw.close();
	SDL_UnlockSurface(img);
}

void Font::load_page(unsigned page){
	auto offset = this->offsets_table[page];
	auto size = this->offsets_table[page + 1] - offset;
	std::vector<Uint8> temp(size);
	this->font_file.clear();
	this->font_file.seekg(offset);
	this->font_file.read((char *)&temp[0], size);
	if (this->font_file.gcount() < size)
		throw CR_Exception("Invalid font file. Incomplete or missing page.");
	auto img = IMG_Load_RW(SDL_RWFromConstMem(&temp[0], size), 1);
	if (!img)
		throw CR_Exception("Font error: couldn't load font subpage. Corrupted font file?");
	{
		auto temp = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA8888, 0);
		SDL_SetSurfaceBlendMode(temp, SDL_BLENDMODE_BLEND);
		SDL_FreeSurface(img);
		img = temp;
	}
	preprocess_image(img);
	auto texture = SDL_CreateTextureFromSurface(this->renderer.get(), img);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_FreeSurface(img);
	this->textures[page].reset(texture, [](SDL_Texture *t){ SDL_DestroyTexture(t); });
}

void Font::compute_rendering_pairs(void (*f)(void *, const rendering_pair &), void *user, const std::string *text, const std::wstring *wtext, int x0, int y0, int wrap_at, double scale){
	int x = x0,
		y = y0;
	size_t n = text ? text->size() : wtext->size();
	const int vertical_advance = int(16 * scale);
	const int halfwidth_size = 8;
	const int fullwidth_size = halfwidth_size * 2;
	for (size_t i = 0; i < n;){
		wchar_t c = text ? (unsigned char)(*text)[i] : (*wtext)[i];
		int character_width = (int)this->width_bitmap[c] + 1;
		if (x + character_width * halfwidth_size * scale > wrap_at){
			x = x0;
			y += vertical_advance;
			continue;
		}
		if (c == '\n'){
			x = x0;
			y += vertical_advance;
		}else{
			rendering_pair rp;
			rp.page = (unsigned)c >> 8;
			rp.src.x = int(c & 0x0F) * fullwidth_size;
			rp.src.y = c & 0xF0;
			rp.src.w = halfwidth_size * character_width;
			rp.src.h = fullwidth_size;
			rp.dst.x = x;
			rp.dst.y = y;
			rp.dst.w = (int)ceil(rp.src.w * scale);
			rp.dst.h = (int)ceil(rp.src.h * scale);
			x += rp.dst.w;
			f(user, rp);
		}
		i++;
	}
}

void Font::draw_text(const std::string *text, const std::wstring *wtext, int x0, int y0, int wrap_at, double scale){
	std::vector<rendering_pair> pairs;
	auto f = [](void *p, const rendering_pair &rp){
		auto &pairs = *(std::vector<rendering_pair> *)p;
		pairs.push_back(rp);
	};
	this->compute_rendering_pairs(f, &pairs, text, wtext, x0, y0, wrap_at, scale);

	std::sort(pairs.begin(), pairs.end(), [](const rendering_pair &a, const rendering_pair &b){ return a.page < b.page; });
	int last_page = -1;
	boost::shared_ptr<SDL_Texture> page;
	for (auto &rp : pairs){
		if (rp.page != last_page){
			page = this->get_page(rp.page);
			last_page = rp.page;
		}
		SDL_RenderCopy(this->renderer.get(), page.get(), &rp.src, &rp.dst);
	}
}

SDL_Rect Font::calculate_bounding_box(const std::wstring &text, int wrap_at, double scale){
	SDL_Rect ret = {0, 0, 0, 0};
	auto f = [](void *p, const rendering_pair &rp){
		auto &ret = *(SDL_Rect *)p;
		ret.w = std::max(ret.w, rp.dst.x + rp.dst.w);
		ret.h = std::max(ret.h, rp.dst.y + rp.dst.h);
	};
	this->compute_rendering_pairs(f, &ret, nullptr, &text, 0, 0, wrap_at, scale);
	return ret;
}
