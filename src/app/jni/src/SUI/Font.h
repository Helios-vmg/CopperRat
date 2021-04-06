/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "../Exception.h"
#include "../Image.h"

#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#include <fstream>
#include <SDL.h>
#include <memory>
#include <type_traits>
#endif

#ifndef __ANDROID__
#define BASE_PATH ""
#else
#define BASE_PATH "/data/data/org.copper.rat/files/"
#endif

struct rendering_pair{
	Uint8 page;
	GPU_Rect src;
	GPU_Rect dst;
	float scale;
};

class Font{
	std::ifstream font_file;
	GPU_Target *target;
	std::vector<bool> width_bitmap;
	std::vector<Uint32> offsets_table;
	std::vector<texture_t> textures;
	
	void initialize_width_bitmap(Uint32 bitmap_offset, Uint32 offsets_table_offset);
	void initialize_offsets_table(Uint32 bitmap_offset, Uint32 offsets_table_offset);
	void load_page(unsigned page);
	texture_t get_page(Uint8 page){
		if (!this->textures[page])
			this->load_page(page);
		return this->textures[page];
	}
	void draw_text(GPU_Target *target, const std::string *, const std::wstring *, int, int, int, double);
	void compute_rendering_pairs(void (*)(void *, const rendering_pair &), void *, const std::string *, const std::wstring *, int, int, int, double);
public:
	Font(GPU_Target *target);
	void draw_text(const std::string &text, int x0, int y0, int wrap_at = INT_MAX, double scale = 1.0){
		this->draw_text(this->target, &text, nullptr, x0, y0, wrap_at, scale);
	}
	void draw_text(const std::wstring &text, int x0, int y0, int wrap_at = INT_MAX, double scale = 1.0){
		this->draw_text(this->target, nullptr, &text, x0, y0, wrap_at, scale);
	}
	void draw_text(GPU_Target *target, const std::wstring &text, int x0, int y0, int wrap_at = INT_MAX, double scale = 1.0){
		this->draw_text(target, nullptr, &text, x0, y0, wrap_at, scale);
	}
	SDL_Rect calculate_bounding_box(const std::wstring &text, int wrap_at = INT_MAX, double scale = 1.0);
	unsigned get_font_height() const{
		return 16;
	}
};
