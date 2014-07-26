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
#endif

void add_slash(std::list<std::vector<DirectoryElement> > &dst){
	std::vector<DirectoryElement> temp;
	DirectoryElement de = {
		L"/",
		1,
	};
	temp.push_back(de);
	dst.push_back(temp);
}

FileBrowser::FileBrowser(SUI *sui, GUIElement *parent, bool select_file, const std::wstring &_initial_directory):
		GUIElement(sui, parent),
		select_file(select_file){
	auto initial_directory = _initial_directory;
FileBrowser__FileBrowser:
	if (!initial_directory.size()){
		add_slash(this->directory_list_stack);
		this->path.push_back(0);
		this->change_directory();
		this->new_initial_directory = L"/";
		return;
	}

	size_t first = 0;
	for (bool lap = 0; ; lap = 1){
		size_t second = initial_directory.find('/', first);
		bool final_step = 0;
		if (second == initial_directory.npos)
			final_step = 1;
		else
			second++;
		std::wstring dir = initial_directory.substr(first, second - first);
		if (!dir.size())
			break;
		first = second;
		if (!lap){
			this->path.push_back(0);
			add_slash(this->directory_list_stack);
		}else{
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
				initial_directory.clear();
				goto FileBrowser__FileBrowser;
			}
			this->path.push_back(index);
		}
		this->change_directory();
	}
	this->new_initial_directory = initial_directory;
}

void FileBrowser::generate_next_list(){
	std::vector<DirectoryElement> list;
	if (!this->select_file){
		DirectoryElement de = {
			L"<Select this directory>",
			1,
		};
		list.push_back(de);
	}
	this->new_initial_directory = this->get_selection_internal(0);
	{
		std::vector<DirectoryElement> temp;
		list_files(temp, this->new_initial_directory, this->select_file ? FilteringType::RETURN_ALL : FilteringType::RETURN_DIRECTORIES);
		for (auto &f : temp){
			if (f.is_dir)
				f.name += '/';
		}
		sort(temp);
		list.reserve(temp.size() + list.size());
		std::copy(temp.begin(), temp.end(), std::back_inserter(list));
	}
	this->directory_list_stack.push_back(list);
	std::vector<std::wstring> temp;
	temp.resize(list.size());
	for (size_t i = 0; i < list.size(); i++)
		temp[i] = list[i].name;
	this->listviews.push_back(lv_t(new ListView(sui, this, temp, (unsigned)this->listviews.size())));
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
					GuiSignal signal;
					signal.type = SignalType::BACK_PRESSED;
					this->parent->gui_signal(signal);
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
		if (i == n)
			break;
		ret = path;
		path += vector[this->path[i]].name;
		i++;
	}
	if (!skip_last)
		ret = path;
	return ret;
}

void FileBrowser::gui_signal(const GuiSignal &_signal){
	GuiSignal signal = _signal;
	size_t n = this->listviews.size();
	if (signal.type != SignalType::LISTVIEW_SIGNAL || signal.data.listview_signal.listview_name != n - 1)
		return;
	signal = *signal.data.listview_signal.signal;
	if (signal.type != SignalType::BUTTON_SIGNAL)
		return;

	auto selection = signal.data.button_signal;
	this->path.push_back(selection);

	bool done = 0;
	if (this->select_file){
		if (!this->directory_list_stack.back()[selection].is_dir)
			done = 1;
	}else if (!selection)
		done = 1;

	if (done){
		signal.type = SignalType::FILE_BROWSER_DONE;
		signal.data.file_browser_success = 1;
		this->parent->gui_signal(signal);
		return;
	}

	this->change_directory();
}

void FileBrowser::update(){
	this->listviews.back()->update();
}

bool FileBrowser::get_input(std::wstring &dst, ControlCoroutine &coroutine, boost::shared_ptr<FileBrowser> self){
	while (1){
		auto signal = coroutine.display(self);
		switch (signal.type){
			case SignalType::BACK_PRESSED:
				return 0;
			case SignalType::FILE_BROWSER_DONE:
				break;
			default:
				continue;
		}
		if (!signal.data.file_browser_success)
			return 0;
		break;
	}
	dst = this->get_selection();
	return 1;
}
