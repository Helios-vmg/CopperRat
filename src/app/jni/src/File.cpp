/*

Copyright (c) 2014, Helios
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "stdafx.h"
#include "File.h"
#include "CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <algorithm>
#endif
#if defined __ANDROID__
#ifndef HAVE_PRECOMPILED_HEADERS
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <dirent.h>
#endif

static void list_files(std::vector<std::pair<std::wstring, bool> > &dst, const std::string &path0){
	auto path = path0;
	while (path.size() > 1 && path.back() == '/')
		path.pop_back();
	auto dir = opendir(path.c_str());
	if (!dir)
		return;
	while (true){
		auto entry = readdir(dir);
		if (!entry)
			break;
		std::string immediate_path = entry->d_name;
		if (immediate_path == "." || immediate_path == "..")
			continue;
		struct stat st_buf;
		bool is_dir;
		if (!stat(immediate_path.c_str(), &st_buf))
			is_dir = S_ISDIR(st_buf.st_mode);
		else
			is_dir = entry->d_type == DT_DIR;
		dst.push_back(std::make_pair(utf8_to_string(immediate_path), is_dir));
	}
	closedir(dir);
}

void list_files(std::vector<DirectoryElement> &dst, const std::wstring &path, FilteringType filter){
	std::string npath = string_to_utf8(path);
	std::vector<std::pair<std::wstring, bool> > temp;
	list_files(temp, npath);
	dst.clear();
	for (auto &s : temp){
		if (s.second && filter == FilteringType::RETURN_FILES || !s.second && filter == FilteringType::RETURN_DIRECTORIES)
			continue;
		dst.push_back(DirectoryElement{ s.first, L"/" + s.first, s.second, });
	}
}

#elif defined WIN32
#ifndef HAVE_PRECOMPILED_HEADERS
#include <Windows.h>
#endif

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
	auto temp_path = path;
	temp_path += '/';
	temp_path += '*';
	HANDLE handle = f.find_first_func(temp_path.c_str(), &data);
	if (handle == INVALID_HANDLE_VALUE)
		return;
	do{
		bool is_dir = check_flag(data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
		if (is_dir && filter == FilteringType::RETURN_FILES || !is_dir && filter == FilteringType::RETURN_DIRECTORIES)
			continue;
		std::basic_string<T> temp = data.cFileName;
		if (temp.size() == 1 && temp[0] == '.')
			continue;
		if (temp.size() == 2 && temp[0] == '.' && temp[1] == '.')
			continue;
		auto name = to_wstring(temp);
		auto path2 = L"/" + name;
		dst.emplace_back(DirectoryElement{ std::move(name), std::move(path2), is_dir, });
	}while (f.find_next_func(handle, &data));
}

void list_files(std::vector<DirectoryElement> &dst, const std::wstring &path, FilteringType filter){
	basic_list_files<wchar_t>(dst, path, filter);
}

void list_files(std::vector<DirectoryElement> &dst, const std::string &path, FilteringType filter){
	basic_list_files<char>(dst, path, filter);
}
#endif

static void find_files_recursively_internal(std::vector<std::wstring> &dst, const std::wstring &path, SortingType st){
	std::vector<DirectoryElement> temp;
	list_files(temp, path, FilteringType::RETURN_ALL);
	sort(temp, st);
	auto *path_with_slash = &path;
	std::wstring temp_path;
	if (path_with_slash->size() && path_with_slash->back() != '/'){
		temp_path = path;
		temp_path += L'/';
		path_with_slash = &temp_path;
	}
	for (auto &de : temp){
		auto full_path = *path_with_slash;
		full_path += de.name;
		if (de.is_dir){
			full_path += '/';
			find_files_recursively_internal(dst, full_path, st);
		}else
			dst.push_back(full_path);
	}
}

void find_files_recursively(std::vector<std::wstring> &dst, const std::wstring &path, SortingType st){
	dst.clear();
	find_files_recursively_internal(dst, path, st);
}

void sort(std::vector<DirectoryElement> &v, SortingType st){
	bool (*f)(const DirectoryElement &a, const DirectoryElement &b);
	if (st == SortingType::DIRECTORIES_FIRST)
		f = [](const DirectoryElement &a, const DirectoryElement &b){
			return a.is_dir > b.is_dir || a.is_dir == b.is_dir && strcmp_case(a.name, b.name) < 0;
		};
	else
		f = [](const DirectoryElement &a, const DirectoryElement &b){
			return a.is_dir < b.is_dir || a.is_dir == b.is_dir && strcmp_case(a.name, b.name) < 0;
		};
	std::sort(v.begin(), v.end(), f);
}

bool file_exists(const std::wstring &path){
	FILE *file = 
#ifdef WIN32
		_wfopen(path.c_str(), L"rb");
#else
		fopen(string_to_utf8(path).c_str(), "rb");
#endif
	if (!file)
		return 0;
	fclose(file);
	return 1;
}
