#ifndef THREADS_H
#define THREADS_H
#include <SDL.h>
#include <queue>
#include <limits>

class Mutex{
	SDL_mutex *mutex;
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();
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
	std::queue<T> queue;
	Mutex mutex;
public:
	unsigned max_size;
	thread_safe_queue(){
		this->max_size = std::numeric_limits<unsigned>::max();
	}
	thread_safe_queue(const thread_safe_queue &b){
		AutoMutex am(b.mutex);
		this->queue = b.queue;
	}
	const thread_safe_queue &operator=(const thread_safe_queue &b){
		AutoMutex am[] = {
			b.mutex,
			this->mutex
		};
		this->queue = b.queue;
	}
#if 0
	void lock(){
		this->mutex.lock();
	}
	void unlock(){
		this->mutex.unlock();
	}
#endif
	void push(const T &e){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size < this->max_size){
					this->queue.push(e);
					return;
				}
			}
			SDL_Delay(10);
		}
		
	}
	bool is_empty(){
		AutoMutex am(this->mutex);
		return this->queue.empty();
	}
	size_t size(){
		AutoMutex am(this->mutex);
		return this->queue.size();
	}
	T &peek(){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size > 0)
					return this->queue.front();
			}
			SDL_Delay(10);
		}
	}
	T *try_peek(){
		AutoMutex am(this->mutex);
		if (!this->queue.size())
			return 0;
		return &this->queue.front();
	}
	T pop(){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size>0){
					T ret = this->queue.front();
					this->queue.pop();
					return ret;
				}
			}
			SDL_Delay(10);
		}
	}
	bool try_pop(T &o){
		AutoMutex am(this->mutex);
		if (!this->queue.size())
			return 0;
		o = this->queue.front();
		this->queue.pop();
		return 1;
	}
	void pop_without_copy(){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size > 0){
					this->queue.pop();
					return;
				}
			}
			SDL_Delay(10);
		}
	}
};

#endif
