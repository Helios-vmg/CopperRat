#include "AudioPlayer.h"
#include "SUI/SUI.h"

int main(int argc, char **argv){
	SDL_Init(SDL_INIT_EVERYTHING);
	{
		AudioPlayer player;
#ifndef __ANDROID__
		SUI sui(player);
		sui.loop();
#endif
	}
	SDL_Quit();
	return 0;
}
