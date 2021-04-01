#include "stdafx.h"
#include "ApplicationState.h"
#include "CommonFunctions.h"
#include "SUI/Font.h"
#include "json.hpp"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <type_traits>
#include <iomanip>
#include <sstream>
#endif

ApplicationState application_state;

#define SAVE_PATH (BASE_PATH "settings.json")

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

#define DEFINE_JSON_STRING(x) const char * const string_##x = #x
#define READ_JSON(src, x) read_json(this->x, src, string_##x)
#define SAVE_JSON(dst, x) save_json(dst, string_##x, this->x)

DEFINE_JSON_STRING(playback_mode);
DEFINE_JSON_STRING(shuffle);
DEFINE_JSON_STRING(last_root);
DEFINE_JSON_STRING(last_browse_directory);
DEFINE_JSON_STRING(current_track);
DEFINE_JSON_STRING(current_time);
DEFINE_JSON_STRING(playlist);
DEFINE_JSON_STRING(shuffle_list);
DEFINE_JSON_STRING(visualization_mode);
DEFINE_JSON_STRING(display_fps);
DEFINE_JSON_STRING(player_indices);
DEFINE_JSON_STRING(current_player);

template <typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
read_json(T &dst, const nlohmann::json &json){
	if (json.is_number_integer())
		dst = json.get<T>();
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, void>::type
read_json(T &dst, const nlohmann::json &json){
	if (json.is_number_float())
		dst = json.get<T>();
}

void read_json(std::wstring &dst, const nlohmann::json &json){
	if (json.is_string())
		dst = utf8_to_string(json.get<std::string>());
}

void read_json(bool &dst, const nlohmann::json &json){
	if (json.is_number())
		dst = json.get<int>() != 0;
	else if (json.is_boolean())
		dst = json.get<bool>();
}

template <typename T>
typename std::enable_if<std::is_enum<T>::value, void>::type
read_json(T &dst, const nlohmann::json &json){
	if (json.is_number())
		dst = (T)json.get<typename std::underlying_type<T>::type>();
}

template <typename T>
void read_json(T &dst, const nlohmann::json &json, const char *name){
	auto it = json.find(name);
	if (it != json.end())
		read_json(dst, *it);
}

template <typename T>
void read_json_array(std::vector<T> &dst, const nlohmann::json &json){
	if (!json.is_array())
		return;
	dst.clear();
	dst.reserve(json.size());
	for (auto &el : json){
		T temp{};
		read_json(temp, el);
		dst.emplace_back(std::move(temp));
	}
}

template <typename T>
void read_json_array(std::vector<T> &dst, const nlohmann::json &json, const char *name){
	auto it = json.find(name);
	if (it != json.end())
		read_json_array(dst, *it);
}

void save_json(nlohmann::json &json, const std::wstring &value){
	json = string_to_utf8(value);
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type
save_json(nlohmann::json &json, T value){
	json = value;
}

void save_json(nlohmann::json &json, bool value){
	json = (int)value;
}

template <typename T>
typename std::enable_if<std::is_enum<T>::value, void>::type
save_json(nlohmann::json &json, T value){
	json = (typename std::underlying_type<T>::type)value;
}

template <typename T>
void save_json(nlohmann::json &json, const char *name, const T &value){
	save_json(json[name], value);
}

template <typename T>
nlohmann::json::array_t save_json_array(const std::vector<T> &src){
	nlohmann::json::array_t ret;
	ret.reserve(src.size());
	for (auto &el : src){
		nlohmann::json temp;
		save_json(temp, el);
		ret.emplace_back(std::move(temp));
	}
	return ret;
}

ApplicationState::ApplicationState(){
	this->reset();
	std::ifstream file(SAVE_PATH);
	if (!file)
		return;
	nlohmann::json json;
	file >> json;

	if (!json.is_object())
		return;

	READ_JSON(json, last_root);
	READ_JSON(json, last_browse_directory);
	READ_JSON(json, visualization_mode);
	READ_JSON(json, display_fps);
	READ_JSON(json, current_player);

	std::vector<int> player_indices;
	read_json_array(player_indices, json, string_player_indices);
	for (auto i : player_indices)
		this->players[i] = {i};

	if (!this->players.size()){
		this->current_player = 0;
		this->players[0] = {0};
	}
}

std::string get_indexed_path(const char *constant_part, int index){
	std::stringstream path;
	path << BASE_PATH << constant_part << std::setw(8) << std::setfill('0') << index << ".json";
	return path.str();
}

std::string PlaybackState::get_path() const{
	return get_indexed_path("playback-", this->index);
}

std::string PlaylistState::get_path() const{
	return get_indexed_path("playlist-", this->index);
}

PlaybackState::PlaybackState(int index): index(index){
	auto file = std::ifstream(this->get_path());
	if (!file)
		return;
	nlohmann::json json;
	file >> json;

	READ_JSON(json, playback_mode);
	READ_JSON(json, shuffle);
	READ_JSON(json, current_track);
	READ_JSON(json, current_time);
}

PlaybackState &PlaybackState::operator=(PlaybackState &&other){
	AutoMutex am(other.mutex);
	this->index = other.index;
	other.index = -1;
	this->no_changes = other.no_changes;
	this->playback_mode = other.playback_mode;
	this->shuffle = other.shuffle;
	this->current_track = other.current_track;
	this->current_time = other.current_time;
	return *this;
}

PlaylistState::PlaylistState(int index): index(index){
	auto file = std::ifstream(this->get_path());
	if (!file)
		return;
	nlohmann::json json;
	file >> json;

	read_json_array(this->playlist_items, json, string_playlist);
	read_json_array(this->shuffle_items, json, string_shuffle_list);
}

PlaylistState &PlaylistState::operator=(PlaylistState &&other){
	AutoMutex am(this->mutex);
	this->index = other.index;
	other.index = -1;
	this->no_changes = other.no_changes;
	this->playlist_items = std::move(other.playlist_items);
	this->shuffle_items = std::move(other.shuffle_items);
	return *this;
}

void PlaybackState::save() const{
	if (this->index < 0)
		return;
	nlohmann::json json;
	{
		AutoMutex am(this->mutex);
		if (this->no_changes)
			return;
		this->no_changes = true;
		
		SAVE_JSON(json, playback_mode);
		SAVE_JSON(json, shuffle);
		SAVE_JSON(json, current_track);
		SAVE_JSON(json, current_time);
	}

	std::ofstream file(this->get_path());
	file << json.dump();
}

void PlaylistState::save() const{
	if (this->index < 0)
		return;
	nlohmann::json json;
	{
		AutoMutex am(this->mutex);
		if (this->no_changes)
			return;
		this->no_changes = true;

		json[string_playlist] = save_json_array(this->playlist_items);
		json[string_shuffle_list] = save_json_array(this->shuffle_items);
	}

	std::ofstream file(this->get_path());
	file << json.dump();
}

void PlayerState::save() const{
	this->playback.save();
	this->playlist.save();
}

void ApplicationState::save(){
	nlohmann::json json;
	{
		std::lock_guard<std::mutex> lg(this->mutex);
		if (this->no_changes)
			return;
		this->no_changes = true;
		
		SAVE_JSON(json, last_root);
		SAVE_JSON(json, last_browse_directory);
		SAVE_JSON(json, visualization_mode);
		SAVE_JSON(json, display_fps);
	}
	
	std::ofstream file(SAVE_PATH);
	file << json.dump();
}

void ApplicationState::reset(){
	this->last_root.clear();
	this->last_browse_directory.clear();
	this->visualization_mode = VisualizationMode::NONE;
	this->display_fps = false;
	this->players.clear();
	this->current_player = 0;
}

void PlaybackState::set_playback_mode(Mode playback_mode){
	AutoMutex am(this->mutex);
	if (this->playback_mode == playback_mode)
		return;
	this->playback_mode = playback_mode;
	this->no_changes = false;
}

void PlaybackState::set_shuffle(bool shuffle){
	AutoMutex am(this->mutex);
	if (this->shuffle == shuffle)
		return;
	this->shuffle = shuffle;
	this->no_changes = false;
}

void PlaybackState::set_current_track(int current_track){
	AutoMutex am(this->mutex);
	this->current_track = current_track;
	this->no_changes = false;
}

void PlaybackState::set_current_time(double current_time){
	AutoMutex am(this->mutex);
	this->current_time = current_time;
	this->no_changes = false;
}

void PlaylistState::set_playlist_items(const std::vector<std::wstring> &playlist_items){
	AutoMutex am(this->mutex);
	if (this->playlist_items == playlist_items)
		return;
	this->playlist_items = playlist_items;
	this->no_changes = false;
}

void PlaylistState::set_shuffle_items(const std::vector<int> &shuffle_items){
	AutoMutex am(this->mutex);
	if (this->shuffle_items == shuffle_items)
		return;
	this->shuffle_items = shuffle_items;
	this->no_changes = false;
}

PlaybackState::Mode PlaybackState::get_playback_mode() const{
	AutoMutex am(this->mutex);
	return this->playback_mode;
}

bool PlaybackState::get_shuffle() const{
	AutoMutex am(this->mutex);
	return this->shuffle;
}

int PlaybackState::get_current_track() const{
	AutoMutex am(this->mutex);
	return this->current_track;
}

double PlaybackState::get_current_time() const{
	AutoMutex am(this->mutex);
	return this->current_time;
}

void PlaylistState::get_playlist_items(std::vector<std::wstring> &playlist_items) const{
	AutoMutex am(this->mutex);
	playlist_items = this->playlist_items;
}

void PlaylistState::get_shuffle_items(std::vector<int> &shuffle_items) const{
	AutoMutex am(this->mutex);
	shuffle_items = this->shuffle_items;
}

void ApplicationState::set_last_root(const std::wstring &last_root){
	{
		std::lock_guard<std::mutex> lg(this->mutex);
		if (this->last_root == last_root)
			return;
		this->last_root = last_root;
		this->no_changes = false;
	}
	this->save();
}

void ApplicationState::set_last_browse_directory(const std::wstring &last_browse_directory){
	{
		std::lock_guard<std::mutex> lg(this->mutex);
		if (this->last_browse_directory == last_browse_directory)
			return;
		this->last_browse_directory = last_browse_directory;
		this->no_changes = false;
	}
	this->save();
}

void ApplicationState::set_visualization_mode(VisualizationMode visualization_mode){
	std::lock_guard<std::mutex> lg(this->mutex);
	if (this->visualization_mode == visualization_mode)
		return;
	this->visualization_mode = visualization_mode;
	this->no_changes = false;
}

void ApplicationState::set_display_fps(bool display_fps){
	std::lock_guard<std::mutex> lg(this->mutex);
	if (this->display_fps == display_fps)
		return;
	this->display_fps = display_fps;
	this->no_changes = false;
}

void ApplicationState::set_current_player_index(int i){
	std::lock_guard<std::mutex> lg(this->mutex);
	if (this->current_player == i)
		return;
	this->current_player = i;
	if (this->players.find(i) == this->players.end())
		this->players[i] = {i};
	this->no_changes = false;
}

std::wstring ApplicationState::get_last_root() const{
	std::lock_guard<std::mutex> lg(this->mutex);
	return this->last_root;
}

std::wstring ApplicationState::get_last_browse_directory() const{
	std::lock_guard<std::mutex> lg(this->mutex);
	return this->last_browse_directory;
}

VisualizationMode ApplicationState::get_visualization_mode() const{
	std::lock_guard<std::mutex> lg(this->mutex);
	return this->visualization_mode;
}

bool ApplicationState::get_display_fps() const{
	std::lock_guard<std::mutex> lg(this->mutex);
	return this->display_fps;
}

const PlayerState &ApplicationState::get_current_player() const{
	const PlayerState *ret;
	{
		std::lock_guard<std::mutex> lg(this->mutex);
		auto it = this->players.find(this->current_player);
		ret = &it->second;
	}
	return *ret;
}

PlayerState &ApplicationState::get_current_player(){
	PlayerState *ret;
	{
		std::lock_guard<std::mutex> lg(this->mutex);
		auto it = this->players.find(this->current_player);
		ret = &it->second;
	}
	return *ret;
}

int ApplicationState::get_current_player_index() const{
	std::lock_guard<std::mutex> lg(this->mutex);
	return this->current_player;
}
