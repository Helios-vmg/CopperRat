#ifndef AUTO_PTR_H
#define AUTO_PTR_H

namespace CR{

struct Null{};

template <typename T, typename D = Null>
class auto_ptr{
	T *p;
	D d;
	auto_ptr(const auto_ptr<T> &){}
public:
	explicit auto_ptr(T *p = nullptr, D d = nullptr): p(p), d(d){}
	~auto_ptr(){
		this->reset();
	}
	void reset(T *p = nullptr){
		this->d(this->p);
		this->p = p;
	}
	T *get(){
		return this->p;
	}
	T &operator*(){
		return *this->p;
	}
	T *operator->(){
		return this->p;
	}
	operator bool() const{
		return this->p != nullptr;
	}
};

template <typename T>
class auto_ptr<T, Null>{
	T *p;
	auto_ptr(const auto_ptr<T> &){}
public:
	explicit auto_ptr(T *p = nullptr): p(p){}
	~auto_ptr(){
		this->reset();
	}
	void reset(T *p = nullptr){
		delete this->p;
		this->p = p;
	}
	T *get(){
		return this->p;
	}
	T &operator*(){
		return *this->p;
	}
	T *operator->(){
		return this->p;
	}
	operator bool() const{
		return this->p != nullptr;
	}
};

}

#if defined __ANDROID__
#define CR_UNIQUE_PTR(x) CR::auto_ptr<x>
#define CR_UNIQUE_PTR2(x, y) CR::auto_ptr<x, y>
#else
#define CR_UNIQUE_PTR(x) std::unique_ptr<x>
#define CR_UNIQUE_PTR2(x, y) std::unique_ptr<x, y>
#endif

#endif
