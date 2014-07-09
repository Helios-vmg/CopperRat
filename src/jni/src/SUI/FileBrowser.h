#include "SUI.h"
#include "../File.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#include <vector>
#endif

class ListView;

class FileBrowser : public GUIElement{
	bool select_file;
	std::vector<size_t> path;
	typedef boost::shared_ptr<ListView> lv_t;
	std::vector<lv_t> listviews;
	std::list<std::vector<DirectoryElement> > directory_list_stack;
	void change_directory();
public:
	FileBrowser(SUI *sui, GUIElement *parent, bool select_file);
	unsigned handle_event(const SDL_Event &);
	void gui_signal(const GuiSignal &);
	void update();
	std::wstring get_selection() const;
};
