/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

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
