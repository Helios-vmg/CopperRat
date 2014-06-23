#ifndef DELETERS_H
#define DELETERS_H

struct SDL_Window_deleter{
	void operator()(SDL_Window *w) const{
		if (w)
			SDL_DestroyWindow(w);
	}
};

inline void SDL_Renderer_deleter_func(SDL_Renderer *r){
	if (r)
		SDL_DestroyRenderer(r);
}

struct SDL_Renderer_deleter{
	void operator()(SDL_Renderer *r) const{
		SDL_Renderer_deleter_func(r);
	}
};

inline void SDL_Surface_deleter_func(SDL_Surface *s){
	if (s)
		SDL_FreeSurface(s);
}

struct SDL_Surface_deleter{
	void operator()(SDL_Surface *s) const{
		SDL_Surface_deleter_func(s);
	}
};

inline void SDL_Texture_deleter_func(SDL_Texture *t){
	if (t)
		SDL_DestroyTexture(t);
}

struct SDL_Texture_deleter{
	void operator()(SDL_Texture *r) const{
		SDL_Texture_deleter_func(r);
	}
};

#endif
