#include "File.h"
#include "CommonFunctions.h"
#if defined __ANDROID__
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <dirent.h>

void list_files(std::vector<std::string> &dst, const std::string &path){
	dst.clear();
	auto dir = opendir(path.c_str());
	if (!dir)
		return;
	while (1){
		auto entry = readdir(dir);
		if (!entry)
			break;
		std::string immediate_path = path + "/" + entry->d_name;
		struct stat st_buf;
		if (stat(immediate_path.c_str(), &st_buf))
			continue;
		if (!S_ISREG(st_buf.st_mode))
			continue;
		dst.push_back(immediate_path);
	}
	closedir(dir);
}

void list_files(std::vector<std::wstring> &dst, const std::wstring &path){
	std::string npath = string_to_utf8(path);
	std::vector<std::string> temp;
	list_files(temp, npath);
	dst.clear();
	for (auto &s : temp)
		dst.push_back(utf8_to_string(s));
}

#elif defined WIN32
#include <Windows.h>

template <typename C>
struct win32_find{
};

template <>
struct win32_find<char>{
	typedef char C;
	typedef WIN32_FIND_DATAA WIN32_FIND_DATA;
	HANDLE find_first_func(const C *a, WIN32_FIND_DATA *b){
		return FindFirstFileA(a, b);
	}
	BOOL find_next_func(HANDLE a, WIN32_FIND_DATA *b){
		return FindNextFileA(a, b);
	}
};

template <>
struct win32_find<wchar_t>{
	typedef wchar_t C;
	typedef WIN32_FIND_DATAW WIN32_FIND_DATA;
	HANDLE find_first_func(const C *a, WIN32_FIND_DATA *b){
		return FindFirstFileW(a, b);
	}
	BOOL find_next_func(HANDLE a, WIN32_FIND_DATA *b){
		return FindNextFileW(a, b);
	}
};

template <typename T>
void basic_list_files(std::vector<std::basic_string<T> > &dst, const std::basic_string<T> &path){
	win32_find<T> f;
	win32_find<T>::WIN32_FIND_DATA data;
	dst.clear();
	HANDLE handle = f.find_first_func(path.c_str(), &data);
	if (handle == INVALID_HANDLE_VALUE)
		return;
	do{
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		dst.push_back(data.cFileName);
	}while (f.find_next_func(handle, &data));
}

void list_files(std::vector<std::wstring> &dst, const std::wstring &path){
	basic_list_files<wchar_t>(dst, path);
}

void list_files(std::vector<std::string> &dst, const std::string &path){
	basic_list_files<char>(dst, path);
}
#endif
