#include "stdafx.h"
#include "AudioPlayer.h"
#include "SUI/SUI.h"
#include "CommonFunctions.h"
#include "File.h"

int main(int argc, char **argv){
	SDL_Init(SDL_INIT_EVERYTHING);
	try{
		AudioPlayer player;
		SUI sui(player);
		sui.loop();
	}catch (const UIInitializationException &e){
		e; //Shut MSVC up about unreferenced local variables.
		__android_log_print(ANDROID_LOG_DEBUG, "C++Exception", "%s", e.desc.c_str());
	}catch (const std::exception &e){
		e; //Shut MSVC up about unreferenced local variables.
		__android_log_print(ANDROID_LOG_DEBUG, "C++Exception", "%s", e.what());
	}
	SDL_Quit();
	return 0;
}
