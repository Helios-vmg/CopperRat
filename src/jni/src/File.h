#ifndef FILE_H
#define FILE_H

#include <vector>
#include <string>

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
#endif
