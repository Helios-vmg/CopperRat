
struct SDL_Window_deleter{
	void operator()(SDL_Window *w) const{
		if (w)
			SDL_DestroyWindow(w);
	}
};

struct SDL_Renderer_deleter{
	void operator()(SDL_Renderer *r) const{
		if (r)
			SDL_DestroyRenderer(r);
	}
};
