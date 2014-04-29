#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H
#include "BasicTypes.h"

inline double s16_to_double(Sint16 x){
	return (x<0) ? (x/32768.0) : (x/32767.0);
}

inline Sint16 double_to_s16(double x){
	return Sint16((x<0) ? (x * 32768.0) : (x * 32767.0));
}

#endif
