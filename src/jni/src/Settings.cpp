#include "stdafx.h"
#include "Settings.h"
#include "CommonFunctions.h"
#include "SUI/Font.h"
#include "tinyxml2.h"

Settings application_settings;

#define SAVE_PATH (BASE_PATH "settings.xml")

Settings::Settings(){
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
	}
}

void Settings::commit(){
	AutoMutex am(this->mutex);
	tinyxml2::XMLDocument doc;
	auto settings = doc.NewElement("settings");
	doc.LinkEndChild(settings);
	settings->SetAttribute("playback_mode", (int)this->playback_mode);
	settings->SetAttribute("shuffle", this->shuffle);
	if (this->last_browse_directory.size())
		settings->SetAttribute("last_browse_directory", string_to_utf8(this->last_browse_directory).c_str());
	doc.SaveFile(SAVE_PATH);
}

void Settings::set_default_values(){
	this->shuffle = 0;
	this->playback_mode = Mode::REPEAT_LIST;
}

void Settings::set_playback_mode(Mode mode){
	AutoMutex am(this->mutex);
	this->playback_mode = mode;
}

void Settings::set_shuffle(bool shuffle){
	AutoMutex am(this->mutex);
	this->shuffle = shuffle;
}

void Settings::set_last_browse_directory(const std::wstring &last_browse_directory){
	{
		AutoMutex am(this->mutex);
		this->last_browse_directory = last_browse_directory;
	}
	this->commit();
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
