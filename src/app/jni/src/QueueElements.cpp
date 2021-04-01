/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "QueueElements.h"
#include "AudioPlayer.h"

void ExternalQueueElement::push(AudioPlayer *player, std::shared_ptr<InternalQueueElement> pointer){
	player->external_queue_out.push(std::static_pointer_cast<ExternalQueueElement>(pointer));
}

AudioCallback_switch_SIGNATURE2(BufferQueueElement::){
	state = this->state;
	const size_t bytes_written = samples_written * bytes_per_sample;
	auto &buffer = this->buffer;
	last_position = buffer.position;
	sample_rate = this->stream_format.freq;
	size_t ctb_res = buffer.copy_to_buffer<Sint16, 2>(stream + bytes_written, len - samples_written * bytes_per_sample);
	samples_written += (memory_sample_count_t)(ctb_res / bytes_per_sample);
	return !buffer.samples();
}

AudioCallback_switch_SIGNATURE2(PlaybackEnd::){
	state = this->state;
	player->notify_playback_end();
	return true;
}

unsigned TotalTimeUpdate::receive(UserInterface &ui){
	return ui.receive(*this);
}

unsigned MetaDataUpdate::receive(UserInterface &ui){
	return ui.receive(*this);
}

unsigned PlaybackStop::receive(UserInterface &ui){
	return ui.receive(*this);
}
