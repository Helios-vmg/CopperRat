/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef FILE_H
#define FILE_H

#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#include <string>
#endif

struct DirectoryElement{
	std::wstring name;
	std::wstring path;
	bool is_dir;
};

enum class FilteringType{
	RETURN_ALL,
	RETURN_FILES,
	RETURN_DIRECTORIES,
};

enum class SortingType{
	FILES_FIRST,
	DIRECTORIES_FIRST,
};

void list_files(std::vector<DirectoryElement> &dst, const std::wstring &path, FilteringType);
void list_files(std::vector<DirectoryElement> &dst, const std::string &path, FilteringType);

void find_files_recursively(std::vector<std::wstring> &dst, const std::wstring &path, SortingType);

void sort(std::vector<DirectoryElement> &, SortingType = SortingType::DIRECTORIES_FIRST);

bool file_exists(const std::wstring &path);
#endif
