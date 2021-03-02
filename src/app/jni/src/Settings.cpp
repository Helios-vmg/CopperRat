#include "stdafx.h"
#include "Settings.h"
#include "CommonFunctions.h"
#include "SUI/Font.h"
#include "tinyxml2.h"

Settings application_settings;

#define SAVE_PATH (BASE_PATH "settings.xml")

std::wstring to_string(VisualizationMode mode){
	switch (mode){
#define CHECK_MODE(x)               \
	case VisualizationMode::x: \
		return L###x
		CHECK_MODE(NONE);
		CHECK_MODE(OSCILLOSCOPE);
		CHECK_MODE(SPECTRUM_LOW);
		CHECK_MODE(SPECTRUM_MID);
		CHECK_MODE(SPECTRUM_HIGH);
		CHECK_MODE(SPECTRUM_MAX);
	}
	assert(0);
	return L"";
}

bool query(int &dst, tinyxml2::XMLElement *element){
	return element->QueryIntText(&dst) == tinyxml2::XML_NO_ERROR;
}

bool query(bool &dst, tinyxml2::XMLElement *element){
	return element->QueryBoolText(&dst) == tinyxml2::XML_NO_ERROR;
}

bool query(double &dst, tinyxml2::XMLElement *element){
	return element->QueryDoubleText(&dst) == tinyxml2::XML_NO_ERROR;
}

bool query(std::wstring &dst, tinyxml2::XMLElement *element){
	const char *s = element->GetText();
	if (!s)
		return 0;
	dst = utf8_to_string(s);
	return 1;
}

template <typename T>
T query(tinyxml2::XMLElement *element){
	T ret;
	if (query(ret, element))
		return ret;
	return T();
}

template <typename T>
void read_list(std::vector<T> &dst, tinyxml2::XMLElement *element){
	dst.clear();
	for (auto *el = element->FirstChildElement(); el; el = el->NextSiblingElement()){
		if (strcmp(el->Name(), "item"))
			continue;
		dst.push_back(query<T>(el));
	}
}

Settings::Settings(): no_changes(1){
	this->set_default_values();
	tinyxml2::XMLDocument doc;
	doc.LoadFile(SAVE_PATH);
	tinyxml2::XMLElement *settings = nullptr;
	for (auto *el = doc.FirstChildElement(); el; el = el->NextSiblingElement()){
		if (!strcmp(el->Name(), "settings")){
			settings = el;
			break;
		}
	}
	if (!settings)
		return;

	for (auto *el = settings->FirstChildElement(); el; el = el->NextSiblingElement()){
		auto name = el->Name();
		if (!strcmp(name, "playback_mode"))
			this->playback_mode = (Mode)query<int>(el);
		else if (!strcmp(name, "shuffle"))
			this->shuffle = query<bool>(el);
		else if (!strcmp(name, "last_browse_directory"))
			this->last_browse_directory = query<std::wstring>(el);
		else if (!strcmp(name, "current_track"))
			this->current_track = query<int>(el);
		else if (!strcmp(name, "current_time"))
			this->current_time = query<double>(el);
		else if (!strcmp(name, "playlist"))
			read_list(this->playlist_items, el);
		else if (!strcmp(name, "shuffle_list"))
			read_list(this->shuffle_items, el);
		else if (!strcmp(name, "visualization_mode"))
			this->visualization_mode = (VisualizationMode)query<int>(el);
		else if (!strcmp(name, "display_fps"))
			this->display_fps = query<bool>(el);
	}
}

tinyxml2::XMLElement *generate_element(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement *parent, const char *name){
	auto ret = doc.NewElement(name);
	parent->LinkEndChild(ret);
	return ret;
}

template <typename T>
void generate_element(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement *parent, const char *name, const T &value){
	auto element = generate_element(doc, parent, name);
	element->SetText(value);
}

void Settings::commit(){
	AutoMutex am(this->mutex);
	if (this->no_changes)
		return;

	tinyxml2::XMLDocument doc;
	auto settings = doc.NewElement("settings");
	doc.LinkEndChild(settings);
	generate_element(doc, settings, "playback_mode", (int)this->playback_mode);
	generate_element(doc, settings, "shuffle", this->shuffle);
	if (this->last_browse_directory.size())
		generate_element(doc, settings, "last_browse_directory", string_to_utf8(this->last_browse_directory).c_str());
	generate_element(doc, settings, "current_track", this->current_track);
	generate_element(doc, settings, "current_time", this->current_time);
	{
		auto playlist = generate_element(doc, settings, "playlist");
		for (auto &s : this->playlist_items)
			generate_element(doc, playlist, "item", string_to_utf8(s).c_str());
	}
	{
		auto shuffle = generate_element(doc, settings, "shuffle_list");
		for (auto i : this->shuffle_items)
			generate_element(doc, shuffle, "item", i);
	}
	generate_element(doc, settings, "visualization_mode", (int)this->visualization_mode);
	generate_element(doc, settings, "display_fps", this->display_fps);
	doc.SaveFile(SAVE_PATH);
	this->no_changes = 1;
}

void Settings::set_default_values(){
	this->shuffle = 0;
	this->playback_mode = Mode::REPEAT_LIST;
	this->current_time = -1;
	this->current_track = -1;
	this->visualization_mode = VisualizationMode::NONE;
	this->display_fps = 0;
}

void Settings::set_playback_mode(Mode mode){
	AutoMutex am(this->mutex);
	this->playback_mode = mode;
	this->no_changes = 0;
}

void Settings::set_shuffle(bool shuffle){
	AutoMutex am(this->mutex);
	this->shuffle = shuffle;
	this->no_changes = 0;
}

void Settings::set_last_browse_directory(const std::wstring &last_browse_directory){
	{
		AutoMutex am(this->mutex);
		this->last_browse_directory = last_browse_directory;
		this->no_changes = 0;
	}
	this->commit();
}

void Settings::set_current_track(int current_track){
	AutoMutex am(this->mutex);
	this->current_track = current_track;
	this->no_changes = 0;
}

void Settings::set_current_time(double current_time){
	AutoMutex am(this->mutex);
	this->current_time = current_time;
	this->no_changes = 0;
}

void Settings::set_playlist_items(const std::vector<std::wstring> &playlist_items){
	AutoMutex am(this->mutex);
	this->playlist_items = playlist_items;
	this->no_changes = 0;
}

void Settings::set_shuffle_items(const std::vector<int> &shuffle_items){
	AutoMutex am(this->mutex);
	this->shuffle_items = shuffle_items;
	this->no_changes = 0;
}

void Settings::set_visualization_mode(VisualizationMode vm){
	AutoMutex am(this->mutex);
	this->visualization_mode = vm;
	this->no_changes = 0;
}

void Settings::set_display_fps(bool dfps){
	AutoMutex am(this->mutex);
	this->display_fps = dfps;
	this->no_changes = 0;
}

Settings::Mode Settings::get_playback_mode(){
	AutoMutex am(this->mutex);
	return this->playback_mode;
}

bool Settings::get_shuffle(){
	AutoMutex am(this->mutex);
	return this->shuffle;
}

std::wstring Settings::get_last_browse_directory(){
	AutoMutex am(this->mutex);
	return this->last_browse_directory;
}

int Settings::get_current_track(){
	AutoMutex am(this->mutex);
	return this->current_track;
}

double Settings::get_current_time(){
	AutoMutex am(this->mutex);
	return this->current_time;
}

void Settings::get_playlist_items(std::vector<std::wstring> &playlist_items){
	AutoMutex am(this->mutex);
	playlist_items = this->playlist_items;
}

void Settings::get_shuffle_items(std::vector<int> &shuffle_items){
	AutoMutex am(this->mutex);
	shuffle_items = this->shuffle_items;
}

VisualizationMode Settings::get_visualization_mode(){
	AutoMutex am(this->mutex);
	return this->visualization_mode;
}

bool Settings::get_display_fps(){
	AutoMutex am(this->mutex);
	return this->display_fps;
}
