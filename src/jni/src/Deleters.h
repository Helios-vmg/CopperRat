
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
