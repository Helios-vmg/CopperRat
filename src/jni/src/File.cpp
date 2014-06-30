#include "File.h"
#include "CommonFunctions.h"
#if defined __ANDROID__
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <dirent.h>

static void list_files(std::vector<std::pair<std::string, bool> > &dst, const std::string &path){
	auto dir = opendir(path.c_str());
	if (!dir)
		return;
	while (1){
		auto entry = readdir(dir);
		if (!entry)
			break;
		std::string immediate_path = entry->d_name;
		struct stat st_buf;
		if (stat(immediate_path.c_str(), &st_buf))
			continue;
		bool is_dir = S_ISDIR(st_buf.st_mode);
		dst.push_back(std::make_pair(utf8_to_string(s.first), is_dir));
	}
	closedir(dir);
}

void list_files(std::vector<std::wstring> &dst, const std::wstring &path){
	std::string npath = string_to_utf8(path);
	std::vector<std::string> temp;
	list_files(temp, npath);
	dst.clear();
	for (auto &s : temp){
		if (s.second && filter == FilteringType::RETURN_FILES)
			continue;
		DirectoryElement de = {
			to_wstring(utf8_to_string(s.first)),
			s.second,
		};
		dst.push_back(de);
	}
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
void basic_list_files(std::vector<DirectoryElement> &dst, const std::basic_string<T> &path, FilteringType filter){
	win32_find<T> f;
	win32_find<T>::WIN32_FIND_DATA data;
	dst.clear();
	HANDLE handle = f.find_first_func(path.c_str(), &data);
	if (handle == INVALID_HANDLE_VALUE)
		return;
	do{
		bool is_dir = check_flag(data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
		if (is_dir && filter == FilteringType::RETURN_FILES)
			continue;
		std::basic_string<T> temp = data.cFileName;
		DirectoryElement de = {
			to_wstring(temp),
			is_dir,
		};
		dst.push_back(de);
	}while (f.find_next_func(handle, &data));
}

void list_files(std::vector<DirectoryElement> &dst, const std::wstring &path, FilteringType filter){
	basic_list_files<wchar_t>(dst, path, filter);
}

void list_files(std::vector<DirectoryElement> &dst, const std::string &path, FilteringType filter){
	basic_list_files<char>(dst, path, filter);
}
#endif
