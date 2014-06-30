#include "SUI.h"
#include "../File.h"
#include <string>
#include <vector>

class ListView;

class FileBrowser : public GUIElement{
	std::vector<size_t> path;
	typedef boost::shared_ptr<ListView> lv_t;
	std::vector<lv_t> listviews;
	std::list<std::vector<DirectoryElement> > directory_list_stack;
public:
	FileBrowser(SUI *sui, GUIElement *parent);
	unsigned handle_event(const SDL_Event &);
	void gui_signal(const GuiSignal &);
	void update();
	const std::wstring &get_selection() const;
};
