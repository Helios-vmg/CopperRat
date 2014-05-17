#include "AudioPlayer.h"
#include "SUI/SUI.h"

int main(int argc, char **argv){
	SDL_Init(SDL_INIT_EVERYTHING);
	{
		AudioPlayer player;
#ifndef __ANDROID__
		SUI sui(player);
		sui.loop();
#else
		player.request_play();
		while (1)
			SDL_Delay(1000);
#endif
	}
	SDL_Quit();
	return 0;
}
