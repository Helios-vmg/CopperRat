#include "Playlist.h"

void Playlist::clear(){
	this->tracks.clear();
	this->shuffle_vector.clear();
	this->current_track = 0;
	this->current_track_is_repeated = 0;
}

void Playlist::insert(const std::vector<std::wstring> &v, size_t p){
	p %= (this->tracks.size() + 1);
	auto n = v.size();
	if (this->shuffle){
		for (auto &i : this->shuffle_vector)
			if (i >= p)
				i += n;
		auto previous_size = this->shuffle_vector.size();
		this->shuffle_vector.resize(previous_size + n);
		for (size_t i = 0; i < n; i++)
			this->shuffle_vector[previous_size + i] = i + p;
		std::random_shuffle(this->shuffle_vector.begin() + previous_size, this->shuffle_vector.end());
	}else if ((this->current_track || this->tracks.size()) && this->current_track >= p)
		this->current_track += n;
	this->tracks.insert(this->tracks.begin() + p, v.begin(), v.end());
}

void Playlist::toggle_shuffle(){
	if (this->shuffle){
		if (this->shuffle_vector.size()){
			this->current_track = this->shuffle_vector[this->current_track];
			this->shuffle_vector.clear();
		}
	}else{
		if (this->tracks.size()){
			this->shuffle_vector.resize(this->tracks.size());
			for (size_t i = 0; i < this->shuffle_vector.size(); i++)
				this->shuffle_vector[i] = i;
			std::swap(this->shuffle_vector[0], this->shuffle_vector[this->current_track]);
			this->current_track = 0;
			std::random_shuffle(this->shuffle_vector.begin() + 1, this->shuffle_vector.end());
		}
	}
	this->shuffle = !this->shuffle;
}

bool Playlist::pop(std::wstring &dst){
	if (this->current_track >= this->tracks.size())
		return 0;
	size_t index = !this->shuffle ? this->current_track : this->shuffle_vector[this->current_track];
	switch (this->mode){
		case PlaybackMode::SINGLE:
			this->current_track++;
			break;
		case PlaybackMode::REPEAT_LIST:
			this->current_track = (this->current_track + 1) % this->tracks.size();
			if (!this->current_track && this->shuffle)
				std::random_shuffle(this->shuffle_vector.begin(), this->shuffle_vector.end());
			break;
		case PlaybackMode::REPEAT_TRACK:
			this->current_track_is_repeated = 1;
			break;
	}
	dst = this->tracks[index];
	return 1;
}

bool Playlist::is_back_possible() const{
	return !(this->shuffle && !this->current_track || this->mode == PlaybackMode::SINGLE);
}

bool Playlist::back(std::wstring &dst){
	if (!this->is_back_possible())
		return 0;
	auto n = this->tracks.size();
	this->current_track = (this->current_track + (n - 1)) % n;
	size_t index = !this->shuffle ? this->current_track : this->shuffle_vector[this->current_track];
	dst = this->tracks[index];
	return 1;
}

Playlist::PlaybackMode Playlist::cycle_mode(){
	if (this->mode == PlaybackMode::REPEAT_TRACK && current_track_is_repeated)
		this->current_track++;
	this->mode = (PlaybackMode)(((int)this->mode + 1) % (int)PlaybackMode::COUNT);
	if (this->mode == PlaybackMode::REPEAT_LIST && this->tracks.size())
		this->current_track %= this->tracks.size();
	return this->mode;
}
