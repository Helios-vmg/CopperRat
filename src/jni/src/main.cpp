#include "AudioPlayer.h"
#include <iostream>
#include <string>

int main(int argc, char **argv){
	AudioPlayer player;
#ifdef WIN32
	while (1){
		std::string word;
		float param;
		std::cin >>word;
		if (word == "seek" && (std::cin >>param)){
			player.relative_seek(param);
		}
	}
#else
	while (1)
		SDL_Delay(1000);
#endif
	return 0;
}
