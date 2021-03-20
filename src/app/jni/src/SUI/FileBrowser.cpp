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

#include "../stdafx.h"
#include "FileBrowser.h"
#include "ListView.h"
#include "../CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <cassert>
#include <string_view>
#endif

void add_slash(std::list<std::vector<DirectoryElement>> &dst, const std::wstring &s){
	std::vector<DirectoryElement> temp;
	temp.emplace_back(DirectoryElement{ s, s, true, });
	dst.emplace_back(std::move(temp));
}

FileBrowser::FileBrowser(SUI *sui, GUIElement *parent, bool select_file, bool can_return, const std::wstring &_root, const std::wstring &_initial_directory):
		GUIElement(sui, parent),
		select_file(select_file){
	auto root = _root;
	std::wstring_view initial_directory = _initial_directory;
FileBrowser__FileBrowser:
	if (!root.size())
		root = L"/";

	add_slash(this->directory_list_stack, root);
	this->path.push_back(0);
	this->change_directory();
	
	size_t first = 0;
	while (initial_directory.size()){
		if (initial_directory.front() == '/'){
			initial_directory = initial_directory.substr(1);
			continue;
		}
		
		size_t second = initial_directory.find('/', first);
		std::wstring dir;
		if (second == initial_directory.npos){
			dir = (std::wstring)initial_directory;
			dir += '/';
			initial_directory = {};
		}else{
			second++;
			dir = (std::wstring)initial_directory.substr(first, second - first);
			initial_directory = initial_directory.substr(second);
		}
		
		int index = -1;
		int i = 0;
		for (auto p : this->directory_list_stack.back()){
			if (p.name == dir){
				index = i;
				break;
			}
			i++;
		}
		if (index < 0){
			this->path.clear();
			this->listviews.clear();
			this->directory_list_stack.clear();
			initial_directory = {};
			goto FileBrowser__FileBrowser;
		}
		this->path.push_back(index);
		this->change_directory();
	}
	this->new_initial_directory = initial_directory;
}

void FileBrowser::generate_next_list(){
	std::vector<DirectoryElement> list;
	if (!this->select_file)
		list.emplace_back(DirectoryElement{ L"<Select this directory>", L"", true, });
	this->new_initial_directory = this->get_selection_internal(false);
	auto dir = this->directory_list_stack.front().front().path + this->new_initial_directory;
	{
		std::vector<DirectoryElement> temp;
		list_files(temp, dir, this->select_file ? FilteringType::RETURN_ALL : FilteringType::RETURN_DIRECTORIES);
		for (auto &f : temp){
			if (f.is_dir)
				f.name += '/';
		}
		sort(temp);
		list.reserve(temp.size() + list.size());
		std::copy(temp.begin(), temp.end(), std::back_inserter(list));
	}
	{
		std::vector<std::wstring> temp;
		temp.reserve(list.size());
		for (auto &i : list)
			temp.push_back(i.name);
		auto n = (unsigned)this->listviews.size();
		auto button = std::make_shared<ListView>(sui, this, temp.begin(), temp.end());
		button->set_on_selection([this, n](size_t button){
			this->gui_signal((unsigned)button);
		});
		this->listviews.emplace_back(std::move(button));
	}
	this->directory_list_stack.emplace_back(std::move(list));
}

void FileBrowser::change_directory(){
	this->generate_next_list();
	this->sui->request_update();
}

unsigned FileBrowser::handle_event(const SDL_Event &e){
	unsigned ret = SUI::NOTHING;
	bool handled = 0;
	if (e.type == SDL_KEYDOWN){
		switch (e.key.keysym.scancode){
			case SDL_SCANCODE_BACKSPACE:
			case SDL_SCANCODE_AC_BACK:
				handled = 1;
				if (this->path.size() == 1){
					if (this->on_cancel)
						this->on_cancel();
					return ret;
				}
				this->listviews.pop_back();
				this->path.pop_back();
				this->directory_list_stack.pop_back();
				this->new_initial_directory = this->get_selection();
				ret |= SUI::REDRAW;
				break;
			default:
				break;
		}
	}
	if (!handled)
		ret |= this->listviews.back()->handle_event(e);
	return ret;
}

std::wstring FileBrowser::get_selection_internal(bool from_outside) const{
	std::wstring path;
	size_t i = 0;
	bool skip_last = from_outside && !this->select_file;
	std::wstring ret;
	auto n = std::min(this->directory_list_stack.size(), this->path.size());
	for (auto &vector : this->directory_list_stack){
		if (from_outside || i){
			if (i == n)
				break;
			ret = path;
			path += vector[this->path[i]].path;
		}
		i++;
	}
	if (!skip_last)
		ret = path;
	return ret;
}

void FileBrowser::gui_signal(unsigned selection){
	this->path.push_back(selection);

	bool done = false;
	if (this->select_file){
		if (!this->directory_list_stack.back()[selection].is_dir)
			done = true;
	}else if (!selection)
		done = true;

	if (done){
		if (this->on_accept)
			this->on_accept(this->get_selection());
		return;
	}

	this->change_directory();
}

void FileBrowser::update(){
	this->listviews.back()->update();
}
