#ifndef FILE_H
#define FILE_H

#include <vector>
#include <string>

void list_files(std::vector<std::wstring> &dst, const std::wstring &path);
void list_files(std::vector<std::string> &dst, const std::string &path);
#endif
