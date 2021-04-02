/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioPlayer.h"
#include "SUI/SUI.h"
#include "CommonFunctions.h"
#include "File.h"
#include "Threads.h"
#include <thread>
#include <locale.h>

#include "Rational.h"

void proper_main(){
	SynchronousEvent event;
	std::thread player_thread;
	AudioPlayer player;
	{
		SUI sui(player);
		player.sui = &sui;
		player.initialize(false);
		sui.initialize();
		player_thread = std::thread([&](){
			player.loop();
			event.wait();
		});
		sui.loop();
	}
	player.request_exit();
	event.set();
	player_thread.join();
}

int main(int argc, char **argv){
	setlocale(LC_ALL, "en.UTF-8");
	SDL_Init(SDL_INIT_EVERYTHING);
	initialize_resources();
	try{
		proper_main();
		__android_log_print(ANDROID_LOG_INFO, "C++main", "%s", "Terminating normally.\n");
	}catch (const std::exception &e){
		__android_log_print(ANDROID_LOG_DEBUG, "C++Exception", "%s", e.what());
	}
	SDL_Quit();
	return 0;
}
