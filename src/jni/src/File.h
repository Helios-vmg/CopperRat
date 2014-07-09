#ifndef FILE_H
#define FILE_H

#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#include <string>
#endif

struct DirectoryElement{
	std::wstring name;
	bool is_dir;
};

enum class FilteringType{
	RETURN_ALL,
	RETURN_FILES,
	RETURN_DIRECTORIES,
};

void list_files(std::vector<DirectoryElement> &dst, const std::wstring &path, FilteringType);
void list_files(std::vector<DirectoryElement> &dst, const std::string &path, FilteringType);

void find_files_recursively(std::vector<std::wstring> &dst, const std::wstring &path);

void sort(std::vector<DirectoryElement> &);
#endif
