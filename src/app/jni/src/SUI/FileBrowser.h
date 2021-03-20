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

#include "SUI.h"
#include "../File.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#include <vector>
#include <functional>
#endif

class ListView;

class FileBrowser : public GUIElement{
	bool select_file;
	std::vector<size_t> path;
	typedef std::shared_ptr<ListView> lv_t;
	std::vector<lv_t> listviews;
	std::list<std::vector<DirectoryElement> > directory_list_stack;
	std::wstring new_initial_directory;
	std::function<void()> on_cancel;
	std::function<void(std::wstring &&)> on_accept;
	
	void change_directory();
	std::wstring get_selection_internal(bool from_outside) const;
	void generate_next_list();
	void gui_signal(unsigned);
public:
	FileBrowser(SUI *sui, GUIElement *parent, bool select_file, bool can_return, const std::wstring &root, const std::wstring &initial_directory);
	unsigned handle_event(const SDL_Event &);
	void update();
	std::wstring get_selection() const{
		return this->get_selection_internal(1);
	}
	const std::wstring &get_new_initial_directory() const{
		return this->new_initial_directory;
	}
	void set_on_cancel(std::function<void()> &&f){
		this->on_cancel = std::move(f);
	}
	void set_on_accept(std::function<void(std::wstring &&)> &&f){
		this->on_accept = std::move(f);
	}
};
