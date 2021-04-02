/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "Threads.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#if defined WIN32
#include <Windows.h>
#elif defined __ANDROID__
#include <unistd.h>
#else
#error Platform not supported!
#endif
#endif

Mutex::Mutex(){
	this->mutex = SDL_CreateMutex();
}

Mutex::~Mutex(){
	SDL_DestroyMutex(this->mutex);
}

Mutex &Mutex::operator=(Mutex &&other){
	this->mutex = other.mutex;
	other.mutex = nullptr;
	return *this;
}

void Mutex::lock(){
	SDL_LockMutex(this->mutex);
}

void Mutex::unlock(){
	SDL_UnlockMutex(this->mutex);
}

SynchronousEvent::SynchronousEvent(){
	this->c = SDL_CreateCond();
}

SynchronousEvent::~SynchronousEvent(){
	SDL_DestroyCond(this->c);
}

SynchronousEvent &SynchronousEvent::operator=(SynchronousEvent &&other){
	this->signalled = other.signalled;
	other.signalled = false;
	this->c = other.c;
	other.c = nullptr;
	this->m = std::move(other.m);
	return *this;
}

void SynchronousEvent::set(){
	AutoMutex am(this->m);
	this->signalled = true;
	SDL_CondSignal(this->c);
}

void SynchronousEvent::wait(){
	AutoMutex am(this->m);
	while (!this->signalled)
		SDL_CondWait(this->c, this->m.get());
	this->signalled = false;
}

void SynchronousEvent::wait(unsigned timeout){
	AutoMutex am(this->m);
	while (!this->signalled)
		SDL_CondWaitTimeout(this->c, this->m.get(), timeout);
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

WorkerThread::WorkerThread(bool low_priority): execute(true), low_priority(low_priority){
	this->thread = std::thread([this](){ this->thread_func(); });
}

WorkerThread::~WorkerThread(){
	this->queue.push([this](){ this->execute = false; });
	this->thread.join();
}

void lower_priority(){
#if defined WIN32
	auto handle = GetCurrentThread();
	SetThreadPriority(handle, THREAD_PRIORITY_IDLE);
	CloseHandle(handle);
#elif defined __ANDROID__
	nice(-20);
#else
#error Platform not supported!
#endif
}

void WorkerThread::thread_func(){
	if (this->low_priority)
		lower_priority();
	while (this->execute){
		auto job = this->queue.pop();
		if (job)
			job();
	}
}
