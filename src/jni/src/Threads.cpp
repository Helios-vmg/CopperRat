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

SynchronousEvent::SynchronousEvent(){
	this->m = SDL_CreateMutex();
	this->c = SDL_CreateCond();
}

SynchronousEvent::~SynchronousEvent(){
	SDL_DestroyMutex(this->m);
	SDL_DestroyCond(this->c);
}

void SynchronousEvent::set(){
	SDL_CondSignal(this->c);
}

void SynchronousEvent::wait(){
	SDL_CondWait(this->c, this->m);
}

void SynchronousEvent::wait(unsigned timeout){
	SDL_CondWaitTimeout(this->c, this->m, timeout);
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
