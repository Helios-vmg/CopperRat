#include "Threads.h"

Mutex::Mutex(){
	this->mutex = SDL_CreateMutex();
}

Mutex::~Mutex(){
	SDL_DestroyMutex(this->mutex);
}

void Mutex::lock(){
	SDL_LockMutex(this->mutex);
}

void Mutex::unlock(){
	SDL_UnlockMutex(this->mutex);
}

void RecursiveMutex::lock(){
	Uint32 tid = SDL_ThreadID();
	{
		AutoMutex am(this->metamutex);
		if (this->lock_count && this->locked_by == tid){
			this->lock_count++;
			return;
		}
	}
	this->mutex.lock();
	this->lock_count++;
	this->locked_by = tid;
}

void RecursiveMutex::unlock(){
	{
		AutoMutex am(this->metamutex);
		this->lock_count--;
		if (this->lock_count)
			return;
	}
	this->mutex.unlock();
}
