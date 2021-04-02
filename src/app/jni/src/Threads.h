/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL.h>
#include <SDL_atomic.h>
#include <queue>
#include <limits>
#include <memory>
#include <thread>
#endif

class Mutex{
	SDL_mutex *mutex;
public:
	Mutex();
	~Mutex();
	Mutex(const Mutex &) = delete;
	Mutex &operator=(const Mutex &) = delete;
	Mutex(Mutex &&other){
		*this = std::move(other);
	}
	Mutex &operator=(Mutex &&);
	void lock();
	void unlock();
	SDL_mutex *get() const{
		return this->mutex;
	}
};

class SynchronousEvent{
	bool signalled = false;
	SDL_cond *c;
	Mutex m;
public:
	SynchronousEvent();
	~SynchronousEvent();
	SynchronousEvent(const SynchronousEvent &) = delete;
	SynchronousEvent &operator=(const SynchronousEvent &) = delete;
	SynchronousEvent(SynchronousEvent &&other){
		*this = std::move(other);
	}
	SynchronousEvent &operator=(SynchronousEvent &&);
	void set();
	void wait();
	void wait(unsigned timeout);
};

template <typename T>
class Future{
	bool set = false;
	SynchronousEvent e;
	T v;
public:
	Future() = default;
	template <typename T2>
	Future(T2 &&initial): v(std::move(initial)){}
	Future(const Future &) = delete;
	Future &operator=(const Future &) = delete;
	Future(Future &&) = delete;
	Future &operator=(Future &&) = delete;
	Future &operator=(T &&value){
		this->v = std::move(value);
		this->e.set();
		return *this;
	}
	T &operator*(){
		if (!this->set){
			this->e.wait();
			this->set = true;
		}
		return this->v;
	}
};

class RecursiveMutex{
	Mutex mutex, metamutex;
	unsigned lock_count;
	Uint32 locked_by;
public:
	RecursiveMutex(): lock_count(0), locked_by(0){}
	void lock();
	void unlock();
};

template <typename T>
class AutoLocker{
	T &lockable;
public:
	AutoLocker(T &l): lockable(l){
		this->lockable.lock();
	}
	~AutoLocker(){
		this->lockable.unlock();
	}
};

typedef AutoLocker<Mutex> AutoMutex;
typedef AutoLocker<RecursiveMutex> AutoRecursiveMutex;

template <typename T>
class thread_safe_queue{
	friend class AutoLocker<thread_safe_queue<T> >;
	std::queue<T> queue;
	mutable Mutex mutex;
	SynchronousEvent popped_event;
	SynchronousEvent pushed_event;
	void lock(){
		this->mutex.lock();
	}
	void unlock(){
		this->mutex.unlock();
	}
public:
	unsigned max_size = std::numeric_limits<unsigned>::max();
	thread_safe_queue() = default;
	thread_safe_queue(const thread_safe_queue &other){
		AutoMutex am(other.mutex);
		this->queue = other.queue;
	}
	const thread_safe_queue &operator=(const thread_safe_queue &other){
		AutoMutex am[] = {
			other.mutex,
			this->mutex
		};
		this->queue = other.queue;
		return *this;
	}
	thread_safe_queue(thread_safe_queue &&other){
		*this = std::move(other);
	}
	thread_safe_queue &operator=(thread_safe_queue &&other){
		this->queue = std::move(queue);
		this->popped_event = std::move(popped_event);
		this->pushed_event = std::move(pushed_event);
		this->max_size = other.max_size;
		other.max_size = std::numeric_limits<unsigned>::max();
		return *this;
	}
	void clear(){
		AutoMutex am(this->mutex);
		this->unlocked_clear();
	}
	void unlocked_clear(){
		while (this->queue.size())
			this->queue.pop();
	}
	void push(const T &e){
		auto copy = e;
		this->push(std::move(copy));
	}
	void push(T &&e){
		while (true){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size < this->max_size){
					this->queue.emplace(std::move(e));
					this->pushed_event.set();
					return;
				}
			}
			this->popped_event.wait();
		}
	}
	void shove(const T &e){
		auto copy = e;
		this->shove(std::move(copy));
	}
	void shove(T &&e){
		AutoMutex am(this->mutex);
		while (this->size() >= this->max_size)
			this->queue.pop();
		this->queue.emplace(std::move(e));
	}
	bool is_empty(){
		AutoMutex am(this->mutex);
		return this->queue.empty();
	}
	bool unlocked_is_empty(){
		return this->queue.empty();
	}
	bool is_full(){
		AutoMutex am(this->mutex);
		return this->queue.size() >= this->max_size;
	}
	size_t size(){
		AutoMutex am(this->mutex);
		return this->queue.size();
	}
	T &peek(){
		while (true){
			{
				AutoMutex am(this->mutex);
				if (!this->queue.empty())
					return this->queue.front();
			}
			this->pushed_event.wait();
		}
	}
	T *try_peek(){
		AutoMutex am(this->mutex);
		return this->unlocked_try_peek();
	}
	T *unlocked_try_peek(){
		if (!this->queue.size())
			return 0;
		return &this->queue.front();
	}
	T pop(){
		while (true){
			{
				AutoMutex am(this->mutex);
				if (!this->queue.empty())
					return this->unlocked_simple_pop();
			}
			this->pushed_event.wait();
		}
	}
	T unlocked_simple_pop(){
		T ret = std::move(this->queue.front());
		this->queue.pop();
		this->popped_event.set();
		return ret;
	}
	bool try_pop(T &o){
		AutoMutex am(this->mutex);
		return this->unlocked_try_pop(o);
	}
	bool unlocked_try_pop(T &o){
		if (!this->queue.size())
			return false;
		o = this->unlocked_simple_pop();
		return true;
	}
	void pop_without_copy(){
		while (true){
			{
				AutoMutex am(this->mutex);
				if (!this->queue.empty()){
					this->queue.pop();
					this->popped_event.set();
					return;
				}
			}
			this->pushed_event.wait();
		}
	}
};

template <typename T>
class Atomic{
	Mutex m;
	T value;
public:
	Atomic(){}
	explicit Atomic(Atomic<T> &a): value(a.get()){}
	explicit Atomic(const T &t): value(t){}
	explicit Atomic(T &&t): value(t){}
	T get(){
		AutoMutex am(this->m);
		return this->value;
	}
	void get(T &dst) const{
		AutoMutex am(this->m);
		dst = this->value;
	}
	void set(const T &t){
		AutoMutex am(this->m);
		this->value = t;
	}
	void set(T &&t){
		AutoMutex am(this->m);
		this->value = t;
	}
};

class WorkerThread;
class WorkerThreadJobHandle;

class CancellableJob{
protected:
	std::atomic<bool> cancelled = false;
public:
	CancellableJob() = default;
	CancellableJob(const CancellableJob &) = delete;
	CancellableJob operator=(const CancellableJob &) = delete;
	CancellableJob(CancellableJob &&) = delete;
	CancellableJob operator=(CancellableJob &&) = delete;
	virtual ~CancellableJob(){}
	void cancel(){
		this->cancelled = true;
	}
	bool is_cancelled() const{
		return this->cancelled.load();
	}
};

class WorkerThread{
	std::thread thread;
	bool low_priority;
	thread_safe_queue<std::function<void()>> queue;
	bool execute;
	void thread_func();
public:
	WorkerThread(bool low_priority = true);
	~WorkerThread();
	template <typename F>
	void attach(F &&f){
		this->queue.push(f);
	}
	template <typename F>
	std::shared_ptr<CancellableJob> attach_cancellable(F &&f){
		auto c = std::make_shared<CancellableJob>();
		this->queue.push([f = std::move(f), c](){
			if (c->is_cancelled())
				return;
			if (f)
				f();
		});
		return c;
	}
};
