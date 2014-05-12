#include "Metadata.h"
#include "CommonFunctions.h"
#include <algorithm>
#include <cctype>

void Metadata::add_vorbis_comment(const void *buffer, size_t length){
	std::wstring comment;
	if (!utf8_to_string(comment, (unsigned char *)buffer, length))
		//invalid UTF-8
		return;
	size_t equals = comment.find('=');
	if (equals == comment.npos)
		//invalid comment
		return;
	auto field_name = comment.substr(0, equals);
	std::transform(field_name.begin(), field_name.end(), field_name.begin(), toupper);
	equals++;
	auto field_value = comment.substr(equals);
	this->add(field_name, field_value);
}
