/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef USERINTERFACE_H
#define USERINTERFACE_H

class TotalTimeUpdate;
class MetaDataUpdate;
class PlaybackStop;

class UserInterface{
public:
	virtual ~UserInterface(){}
	virtual unsigned receive(TotalTimeUpdate &) = 0;
	virtual unsigned receive(MetaDataUpdate &) = 0;
	virtual unsigned receive(PlaybackStop &) = 0;
};

#endif
