#include "AudioPlayer.h"

void AudioCallback(void *udata, Uint8 *stream, int len){
	AudioPlayer *player = (AudioPlayer *)udata;
	const unsigned bytes_per_sample = 2 * 2;

	memory_sample_count_t samples_written = 0;
	while ((unsigned)len > samples_written * bytes_per_sample){
		audio_buffer_t *buffer_pointer;
		size_t bytes_written = samples_written * bytes_per_sample;
		buffer_pointer = player->queue.try_peek();
		if (!buffer_pointer){
			memset(stream + bytes_written, 0, len - bytes_written);
			return;
		}
		audio_buffer_t &buffer = *buffer_pointer;
		size_t ctb_res = buffer.copy_to_buffer<Sint16, 2>(stream + bytes_written, len - samples_written * bytes_per_sample);
		samples_written += (memory_sample_count_t)(ctb_res / bytes_per_sample);
		if (!buffer.samples()){
			buffer = player->queue.pop();
			buffer.free();
		}
	}
}

int main(int argc, char **argv){
	AudioPlayer player;
#ifndef PROFILING
	SDL_Init(SDL_INIT_AUDIO);
	SDL_AudioSpec specs;
	specs.freq = 44100;
	specs.format = AUDIO_S16SYS;
	specs.channels = 2;
	specs.samples = DEFAULT_BUFFER_SIZE;
	specs.callback = AudioCallback;
	specs.userdata = &player;
	if (SDL_OpenAudio(&specs, 0) < 0)
		return -1;
	SDL_PauseAudio(0);
	while (1)
		SDL_Delay(1000);
#endif
	return 0;
}
