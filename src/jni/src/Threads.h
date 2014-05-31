#ifndef THREADS_H
#define THREADS_H
#include <SDL.h>
#include <SDL_atomic.h>
#include <queue>
#include <limits>
#include <boost/shared_ptr.hpp>

class Mutex{
	SDL_mutex *mutex;
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();
};

class SynchronousEvent{
	SDL_cond *c;
	SDL_mutex *m;
public:
	SynchronousEvent();
	~SynchronousEvent();
	void set();
	void wait();
	void wait(unsigned timeout);
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
	Mutex mutex;
	SynchronousEvent popped_event;
	SynchronousEvent pushed_event;
	void lock(){
		this->mutex.lock();
	}
	void unlock(){
		this->mutex.unlock();
	}
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
	void clear(){
		AutoMutex am(this->mutex);
		this->unlocked_clear();
	}
	void unlocked_clear(){
		while (this->queue.size())
			this->queue.pop();
	}
	void push(const T &e){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size < this->max_size){
					this->queue.push(e);
					this->pushed_event.set();
					return;
				}
			}
			this->popped_event.wait();
		}
	}
	void shove(const T &e){
		AutoMutex am(this->mutex);
		while (this->size() >= this->max_size)
			this->queue.pop();
		this->queue.push(e);
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
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size > 0)
					return this->queue.front();
			}
			this->pushed_event.wait();
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
				if (this->queue.size() > 0){
					return this->unlocked_simple_pop();
				}
			}
			this->pushed_event.wait();
		}
	}
	T unlocked_simple_pop(){
		T ret = this->queue.front();
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
			return 0;
		o = this->unlocked_simple_pop();
		return 1;
	}
	void pop_without_copy(){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size > 0){
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

class WorkerThreadJob{
protected:
	WorkerThread *worker;
	int id;
	Atomic<bool> cancelled;
public:
	WorkerThreadJob(): id(0), cancelled(0), worker(0){}
	virtual ~WorkerThreadJob(){}
	void cancel(){
		this->cancelled.set(1);
	}
	bool was_cancelled(){
		return this->cancelled.get();
	}
	virtual void perform(WorkerThread &) = 0;
	void set_id(int id){
		this->id = id;
	}
	int get_id() const{
		return this->id;
	}
};

class SyncWorkerThreadJob : public WorkerThreadJob{
protected:
	SynchronousEvent finished;
	virtual void sync_perform(WorkerThread &) = 0;
public:
	virtual ~SyncWorkerThreadJob(){}
	void perform(WorkerThread &wt){
		this->sync_perform(wt);
		this->finished.set();
	}
	//Call from the non-worker thread.
	void wait(){
		this->finished.wait();
	}
};

class WorkerThreadJobHandle{
	boost::shared_ptr<WorkerThreadJob> job;
public:
	WorkerThreadJobHandle(boost::shared_ptr<WorkerThreadJob> job): job(job){}
	~WorkerThreadJobHandle(){
		job->cancel();
	}
	int get_id() const{
		return this->job->get_id();
	}
};

class WorkerThread{
	SDL_Thread *sdl_thread;
	bool low_priority;
	thread_safe_queue<boost::shared_ptr<WorkerThreadJob> > queue;
	bool execute;
	SDL_atomic_t next_id;
	static int _thread(void *_this){
		((WorkerThread *)_this)->thread();
		return 0;
	}
	void thread();
	class TerminateJob : public SyncWorkerThreadJob{
		void sync_perform(WorkerThread &wt){
			wt.kill();
		}
	};
	boost::shared_ptr<WorkerThreadJob> current_job;
public:
	WorkerThread(bool low_priority = 1);
	~WorkerThread();
	void kill(){
		this->execute = 0;
	}
	boost::shared_ptr<WorkerThreadJobHandle> attach(boost::shared_ptr<WorkerThreadJob>);
	boost::shared_ptr<WorkerThreadJob> get_current_job() const{
		return this->current_job;
	}
};

#endif
