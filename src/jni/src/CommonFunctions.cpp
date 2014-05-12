#include "CommonFunctions.h"
#include <iomanip>

void parse_into_hms(std::ostream &stream, double total_seconds){
	if (total_seconds < 0){
		stream <<"??:??";
		return;
	}
	int seconds = (int)fmod(total_seconds, 60);
	total_seconds = (total_seconds - seconds) / 60.0;
	int minutes = (int)fmod(total_seconds, 60);
	total_seconds = (total_seconds - minutes) / 60.0;
	int hours = (int)total_seconds;
	if (hours > 0)
		stream <<std::setw(2)<<std::setfill('0')<<hours<<":";
	stream <<std::setw(2)<<std::setfill('0')<<minutes<<":"<<std::setw(2)<<std::setfill('0')<<seconds;
}
