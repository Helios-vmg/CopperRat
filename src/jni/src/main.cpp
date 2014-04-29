#include "AudioPlayer.h"

void AudioCallback(void *udata, Uint8 *stream, int len){
	AudioPlayer *player = (AudioPlayer *)udata;
	audio_buffer_t dst_buffer;
	dst_buffer.data = (sample_t *)stream;
	dst_buffer.channel_count = 2;
	const unsigned bytes_per_sample = dst_buffer.channel_count * 2;
	dst_buffer.sample_count = len / bytes_per_sample;

	unsigned samples_written = 0;
	while ((unsigned)len > samples_written * bytes_per_sample){
		audio_buffer_t *buffer_pointer;
		size_t bytes_written = samples_written * bytes_per_sample;
		buffer_pointer = player->queue.try_peek();
		if (!buffer_pointer){
			memset(stream + bytes_written, 0, len - bytes_written);
			return;
		}
		audio_buffer_t &buffer = *buffer_pointer;
		unsigned samples_to_copy = std::min(buffer.samples_produced - buffer.samples_consumed, len / bytes_per_sample - samples_written);
		memcpy(stream + bytes_written, buffer[buffer.samples_consumed], samples_to_copy* bytes_per_sample);
		buffer.samples_consumed += samples_to_copy;
		samples_written += samples_to_copy;
		if (buffer.samples_consumed == buffer.samples_produced){
			buffer = player->queue.pop();
			buffer.free();
		}
	}
}

int main(int argc, char **argv){
	//const size_t N=1<<24;
	//char *raw_buffer=new char[N];
	AudioPlayer player;
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
}
