#ifndef FONT_H
#define FONT_H
#include <vector>
#include <fstream>
#include <SDL.h>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits.hpp>
#include "../Exception.h"

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
		if (!this->textures[page].get())
			this->load_page(page);
		return this->textures[page];
	}
	void draw_text(const std::string *, const std::wstring *, int, int);
public:
	Font(boost::shared_ptr<SDL_Renderer> renderer);
	void draw_text(const std::string &text, int x0, int y0){
		this->draw_text(&text, nullptr, x0, y0);
	}
	void draw_text(const std::wstring &text, int x0, int y0){
		this->draw_text(nullptr, &text, x0, y0);
	}
};

#endif
