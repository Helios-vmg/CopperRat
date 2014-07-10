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

#ifndef FONT_H
#define FONT_H

#include "../Exception.h"

#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#include <fstream>
#include <SDL.h>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits.hpp>
#endif

#ifndef __ANDROID__
#define BASE_PATH
#else
#define BASE_PATH "/data/data/org.copper.rat/files/"
#endif

struct rendering_pair{
	Uint8 page;
	SDL_Rect src;
	SDL_Rect dst;
};

class Font{
	std::ifstream font_file;
	boost::shared_ptr<SDL_Renderer> renderer;
	std::vector<bool> width_bitmap;
	std::vector<Uint32> offsets_table;
	std::vector<boost::shared_ptr<SDL_Texture> > textures;
	void initialize_width_bitmap(Uint32 bitmap_offset, Uint32 offsets_table_offset);
	void initialize_offsets_table(Uint32 bitmap_offset, Uint32 offsets_table_offset);
	void load_page(unsigned page);
	boost::shared_ptr<SDL_Texture> get_page(Uint8 page){
		if (!this->textures[page])
			this->load_page(page);
		return this->textures[page];
	}
	void draw_text(const std::string *, const std::wstring *, int, int, int, double);
	void compute_rendering_pairs(void (*)(void *, const rendering_pair &), void *, const std::string *, const std::wstring *, int, int, int, double);
public:
	Font(boost::shared_ptr<SDL_Renderer> renderer);
	void draw_text(const std::string &text, int x0, int y0, int wrap_at = INT_MAX, double scale = 1.0){
		this->draw_text(&text, nullptr, x0, y0, wrap_at, scale);
	}
	void draw_text(const std::wstring &text, int x0, int y0, int wrap_at = INT_MAX, double scale = 1.0){
		this->draw_text(nullptr, &text, x0, y0, wrap_at, scale);
	}
	SDL_Rect calculate_bounding_box(const std::wstring &text, int wrap_at = INT_MAX, double scale = 1.0);
	unsigned get_font_height() const{
		return 16;
	}
};

#endif
