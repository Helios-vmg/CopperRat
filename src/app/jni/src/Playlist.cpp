/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "Playlist.h"
#include "File.h"
#include "Decoder.h"
#include "CommonFunctions.h"
#include "ApplicationState.h"
#include "XorShift128.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <fstream>
#endif

thread_local XorShift128 xorshift128;
thread_local UniformGeneratorSelector<size_t>::type<XorShift128> size_t_gen(xorshift128);

std::wstring to_string(PlaybackMode mode){
	switch (mode){
#define CHECK_MODE(x) case PlaybackMode::x: return L###x
		CHECK_MODE(SINGLE);
		CHECK_MODE(REPEAT_LIST);
		CHECK_MODE(REPEAT_TRACK);
	}
	assert(0);
	return L"";
}

template <typename It>
void playlist_random_shuffle(It begin, It end){
	auto n = end - begin;
	auto n1 = begin + n / 3;
	auto n2 = begin + n * 2 / 3;
	special_random_shuffle(begin, n2, n1, size_t_gen);
	cr_random_shuffle(n1, end, size_t_gen);
}

Playlist::Playlist(PlayerState &state): state(&state){
	auto &playback = this->state->get_playback();
	this->mode = playback.get_playback_mode();
	this->shuffle = playback.get_shuffle();
	this->current_track = playback.get_current_track();

	auto &playlist = this->state->get_playlist();
	playlist.get_playlist_items(this->tracks);
	playlist.get_shuffle_items(this->shuffle_vector);
}

Playlist &Playlist::operator=(Playlist &&other){
	this->state = other.state;
	other.state = nullptr;
	this->current_track = other.current_track;
	this->mode = other.mode;
	this->shuffle = other.shuffle;
	this->tracks = std::move(other.tracks);
	this->shuffle_vector = std::move(other.shuffle_vector);
	return *this;
}

Playlist::~Playlist(){
	if (this->state)
		this->save();
}

void Playlist::forget_state(){
	this->state = nullptr;
}

void Playlist::clear(){
	this->tracks.clear();
	this->shuffle_vector.clear();
	this->current_track = -1;
}

void Playlist::save(){
	auto &playback = this->state->get_playback();
	playback.set_playback_mode(this->mode);
	playback.set_shuffle(this->shuffle);
	playback.set_current_track(this->current_track);
	
	auto &playlist = this->state->get_playlist();
	playlist.set_playlist_items(this->tracks);
	playlist.set_shuffle_items(this->shuffle_vector);
	this->state->save();
}

void Playlist::insert(const std::vector<std::wstring> &v, size_t p){
	p %= (this->tracks.size() + 1);
	const auto n = v.size();
	if (this->shuffle){
		for (auto &i : this->shuffle_vector)
			if (i >= p)
				i += (int)n;
		auto previous_size = this->shuffle_vector.size();
		this->shuffle_vector.resize(previous_size + n);
		for (size_t i = 0; i < n; i++)
			this->shuffle_vector[previous_size + i] = (int)(i + p);
		cr_random_shuffle(this->shuffle_vector.begin() + previous_size, this->shuffle_vector.end(), size_t_gen);
	}
	if (this->current_track < 0)
		this->current_track = 0;
	else if (this->current_track >= p)
		this->current_track += (int)n;
	this->tracks.insert(this->tracks.begin() + p, v.begin(), v.end());
	this->save();
}

bool Playlist::toggle_shuffle(){
	if (this->shuffle){
		if (this->shuffle_vector.size()){
			this->current_track = this->shuffle_vector[this->current_track];
			this->shuffle_vector.clear();
		}
	}else{
		if (this->tracks.size()){
			this->shuffle_vector.resize(this->tracks.size());
			for (size_t i = 0; i < this->shuffle_vector.size(); i++)
				this->shuffle_vector[i] = (int)i;
			std::swap(this->shuffle_vector[0], this->shuffle_vector[this->current_track]);
			this->current_track = 0;
			cr_random_shuffle(this->shuffle_vector.begin() + 1, this->shuffle_vector.end(), size_t_gen);
		}
	}
	this->shuffle = !this->shuffle;
	this->save();
	return this->shuffle;
}

bool Playlist::get_current_track(std::wstring &dst){
	if (this->at_null_position())
		return false;
	size_t index = !this->shuffle ? this->current_track : this->shuffle_vector[this->current_track];
	dst = this->tracks[index];
	return true;
}

bool Playlist::next(bool by_user){
	if (this->at_null_position())
		return 0;
	auto mode = this->mode;
	if (by_user && mode == PlaybackMode::REPEAT_TRACK)
		mode = PlaybackMode::REPEAT_LIST;
	switch (mode){
		case PlaybackMode::SINGLE:
			this->current_track++;
			break;
		case PlaybackMode::REPEAT_LIST:
			this->current_track = (this->current_track + 1) % this->tracks.size();
			if (!this->current_track && this->shuffle)
				playlist_random_shuffle(this->shuffle_vector.begin(), this->shuffle_vector.end());
			break;
		case PlaybackMode::REPEAT_TRACK:
			break;
	}
	this->state->get_playback().set_current_track(this->current_track);
	return 1;
}

bool Playlist::back(){
	if (!this->is_back_possible())
		return 0;
	auto n = this->tracks.size();
	this->current_track = (int)(((size_t)this->current_track + (n - 1)) % n);
	return 1;
}

bool Playlist::is_back_possible() const{
	if (this->at_null_position())
		return 0;
	if (!this->current_track && (this->shuffle || this->mode == PlaybackMode::SINGLE))
		return 0;
	return 1;
}

PlaybackMode Playlist::cycle_mode(){
	this->mode = (PlaybackMode)(((int)this->mode + 1) % (int)PlaybackMode::COUNT);
	if (this->mode == PlaybackMode::REPEAT_LIST && this->tracks.size())
		this->current_track %= this->tracks.size();
	return this->mode;
}

void Playlist::load(bool file, const std::wstring &path){
	this->clear();
	std::vector<std::wstring> list;
	if (file){
		auto ext = get_extension(path);
		if (ext == L"pls" || ext == L"m3u"){
			this->load_playlist(path);
			return;
		}
		if (!format_is_supported(path))
			return;
		list.push_back(path);
	}else{
		std::vector<std::wstring> files;
		find_files_recursively(files, path, SortingType::FILES_FIRST);
		filter_list_by_supported_formats(list, files);
	}
	this->insert(list, 0);
}

void Playlist::append(bool file, const std::wstring &path){
}

void Playlist::load_playlist(const std::wstring &path){
	auto utf8 = string_to_utf8(path);
	std::ifstream file(utf8.c_str(), std::ios::binary);
	std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	std::vector<std::wstring> paths;
	auto container = get_contaning_directory(path);
	size_t first = 0;
	while (first < data.size()){
		size_t second = data.find_first_of("\n\r", first);
		std::string line = data.substr(first, second - first);
		first = second + (second != data.npos);

		if (!line.size() || line[0] == '#')
			continue;
		normalize_slashes(line);
		std::wstring wide = utf8_to_string(line);
		if (path_is_rooted(wide))
			paths.push_back(wide);
		else
			paths.push_back(container + wide);
	}
	this->insert(paths, 0);
}
