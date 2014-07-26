#include "stdafx.h"
#include "Settings.h"
#include "CommonFunctions.h"
#include "SUI/Font.h"
#include "tinyxml2.h"

Settings application_settings;

#define SAVE_PATH (BASE_PATH "settings.xml")

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

	for (auto *el = settings->FirstAttribute(); el; el = el->Next()){
		auto name = el->Name();
		if (!strcmp(name, "playback_mode"))
			this->playback_mode = (Mode)el->IntValue();
		else if (!strcmp(name, "shuffle"))
			this->shuffle = el->BoolValue();
		else if (!strcmp(name, "last_browse_directory"))
			this->last_browse_directory = utf8_to_string(el->Value());
		else if (!strcmp(name, "current_track"))
			this->current_track = el->IntValue();
		else if (!strcmp(name, "current_time"))
			this->current_time = el->DoubleValue();
		else if (!strcmp(name, "expensive_gfx"))
			this->expensive_gfx = el->BoolValue();
	}
	for (auto *el = settings->FirstChildElement(); el; el = el->NextSiblingElement()){
		auto name = el->Name();
		if (!strcmp(name, "playlist_item"))
			this->playlist_items.push_back(utf8_to_string(el->Attribute("value")));
		else if (!strcmp(name, "shuffle_item"))
			this->shuffle_items.push_back(el->IntAttribute("value"));
	}
}

void Settings::commit(){
	AutoMutex am(this->mutex);
	if (this->no_changes)
		return;

	tinyxml2::XMLDocument doc;
	auto settings = doc.NewElement("settings");
	doc.LinkEndChild(settings);
	settings->SetAttribute("playback_mode", (int)this->playback_mode);
	settings->SetAttribute("shuffle", this->shuffle);
	if (this->last_browse_directory.size())
		settings->SetAttribute("last_browse_directory", string_to_utf8(this->last_browse_directory).c_str());
	settings->SetAttribute("current_track", this->current_track);
	settings->SetAttribute("current_time", this->current_time);
	settings->SetAttribute("expensive_gfx", this->expensive_gfx);
	for (auto &s : this->playlist_items){
		auto playlist_item = doc.NewElement("playlist_item");
		settings->LinkEndChild(playlist_item);
		playlist_item->SetAttribute("value", string_to_utf8(s).c_str());
	}
	for (auto i : this->shuffle_items){
		auto shuffle_item = doc.NewElement("shuffle_item");
		settings->LinkEndChild(shuffle_item);
		shuffle_item->SetAttribute("value", i);
	}
	doc.SaveFile(SAVE_PATH);
	this->no_changes = 1;
}

void Settings::set_default_values(){
	this->shuffle = 0;
	this->playback_mode = Mode::REPEAT_LIST;
	this->current_time = 0;
	this->current_track = -1;
	this->expensive_gfx = 1;
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

void Settings::set_expensive_gfx(bool expensive_gfx){
	AutoMutex am(this->mutex);
	this->expensive_gfx = expensive_gfx;
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

bool Settings::get_expensive_gfx(){
	AutoMutex am(this->mutex);
	return this->expensive_gfx;
}
