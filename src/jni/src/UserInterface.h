#ifndef USERINTERFACE_H
#define USERINTERFACE_H

class TotalTimeUpdate;
class MetaDataUpdate;

class UserInterface{
public:
	virtual ~UserInterface(){}
	virtual unsigned receive(TotalTimeUpdate &) = 0;
	virtual unsigned receive(MetaDataUpdate &) = 0;
};

#endif
