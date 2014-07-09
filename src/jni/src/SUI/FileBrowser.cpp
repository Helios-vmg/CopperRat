#include "../stdafx.h"
#include "FileBrowser.h"
#include "ListView.h"
#include "../CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <cassert>
#endif

FileBrowser::FileBrowser(SUI *sui, GUIElement *parent, bool select_file): GUIElement(sui, parent), select_file(select_file){
	this->path.push_back(0);
	{
		std::vector<DirectoryElement> temp;
		DirectoryElement de = {
			L"/",
			1,
		};
		temp.push_back(de);
		this->directory_list_stack.push_back(temp);
	}
	this->change_directory();
}

void FileBrowser::change_directory(){
	std::vector<DirectoryElement> list;
	{
		std::vector<DirectoryElement> temp;
		list_files(temp, this->get_selection_internal(0), this->select_file ? FilteringType::RETURN_ALL : FilteringType::RETURN_DIRECTORIES);
		list.reserve(temp.size() + list.size());
		std::copy(temp.begin(), temp.end(), std::back_inserter(list));
	}
	sort(list);
	for (auto &f : list){
		if (f.is_dir)
			f.name += '/';
	}
	if (!this->select_file){
		DirectoryElement de = {
			L"<Select this directory>",
			1,
		};
		list.insert(list.begin(), de);
	}
	this->directory_list_stack.push_back(list);
	std::vector<std::wstring> temp;
	temp.resize(list.size());
	for (size_t i = 0; i < list.size(); i++)
		temp[i] = list[i].name;
	this->listviews.push_back(lv_t(new ListView(sui, this, temp, (unsigned)this->listviews.size())));
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
					signal.type = SignalType::FILE_BROWSER_DONE;
					signal.data.file_browser_success = 0;
					this->parent->gui_signal(signal);
					return ret;
				}
				this->listviews.pop_back();
				this->path.pop_back();
				this->directory_list_stack.pop_back();
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
	assert(this->directory_list_stack.size() == this->path.size());
	std::wstring path;
	size_t i = 0;
	bool skip_last = from_outside && !this->select_file;
	std::wstring ret;
	for (auto &vector : this->directory_list_stack){
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
