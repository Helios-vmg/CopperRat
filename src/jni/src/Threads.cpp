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

WorkerThread::WorkerThread(bool low_priority): execute(1), low_priority(low_priority){
	this->next_id.value = 0;
	this->sdl_thread = SDL_CreateThread(_thread, "WorkerThread", this);
}

WorkerThread::~WorkerThread(){
	{
		boost::shared_ptr<TerminateJob> tj(new TerminateJob);
		auto handle = this->attach(tj);
		tj->wait();
	}
	SDL_WaitThread(this->sdl_thread, 0);
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

void WorkerThread::thread(){
	if (this->low_priority)
		lower_priority();
	while (this->execute){
		auto job = this->queue.pop();
		if (job->was_cancelled())
			continue;
		this->current_job = job;
		job->perform(*this);
	}
}

boost::shared_ptr<WorkerThreadJobHandle> WorkerThread::attach(boost::shared_ptr<WorkerThreadJob> job){
	job->set_id(SDL_AtomicAdd(&this->next_id, 1));
	boost::shared_ptr<WorkerThreadJobHandle> ret(new WorkerThreadJobHandle(job));
	this->queue.push(job);
	return ret;
}
