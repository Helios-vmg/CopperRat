#include "FileBrowser.h"

FileBrowser::FileBrowser(SUI *sui, GUIElement *parent): GUIElement(sui, parent){
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
	this->path.resize(1);
	std::vector<std::wstring> list;
	{
		list_files(l, "/", FilteringType::RETURN_ALL);
		for (auto &f : l){
			list.push_back(f.name);
			if (f.is_dir)
				list.back() += '/';
		}
	}
}

unsigned FileBrowser::handle_event(const SDL_Event &e){
	unsigned ret = SUI::NOTHING;
	switch (e.key.keysym.scancode){
		case SDL_SCANCODE_AC_BACK:
			/*
			if (!this->listviews.size())

			if (this->current_element.size() > 1){
				this->current_element.pop_back();
				ret |= REDRAW;
			}
			*/
			break;
	}
	return ret;
}

void FileBrowser::update(){
}
