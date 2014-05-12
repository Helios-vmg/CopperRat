#ifndef METADATA_H
#define METADATA_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

class Metadata{
	std::map<std::wstring, std::wstring> map;
public:
	void add(const std::wstring &key, std::wstring &value){
		this->map[key] = value;
	}
	void add_vorbis_comment(const void *, size_t);
	template <typename F>
	void iterate(F &f){
		for (auto &p : this->map){
			f(p.first, p.second);
		}
	}
	boost::shared_ptr<Metadata> clone(){
		return boost::shared_ptr<Metadata>(new Metadata(*this));
	}
};

#endif
