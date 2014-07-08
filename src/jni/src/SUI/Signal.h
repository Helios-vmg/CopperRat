enum class SignalType{
	QUIT,
	BACK_PRESSED,
	BUTTON_SIGNAL,
	LISTVIEW_SIGNAL,
	MAINSCREEN_LOAD,
	FILE_BROWSER_DONE,
};

struct GuiSignal{
	SignalType type;
	union{
		unsigned button_signal;
		struct{
			unsigned listview_name;
			const GuiSignal *signal;
		} listview_signal;
		bool file_browser_success;
	} data;
};
