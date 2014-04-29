#include <iostream>
#include <fstream>
#include <cstdio>
#include <SDL.h>
#include <cmath>
#include <cassert>
#include <queue>
#include <limits>
#include <utility>
//#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#ifdef WIN32
#include <Windows.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif
#endif

struct Clock{
	typedef double t;
private:
	t data;
public:
	Clock();
	t operator()() const;
};

Clock::Clock(){
	this->data = 0;
#ifdef WIN32
	LARGE_INTEGER li;
	this->data = (!QueryPerformanceFrequency(&li)) ? 0 : t(1000) / t(li.QuadPart);
#endif
}

Clock::t Clock::operator()() const{
#ifdef WIN32
	if (!this->data)
		return 0;
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return t(li.QuadPart) * this->data;
#else
	return 0;
#endif
}

inline double s16_to_double(Sint16 x){
	return (x<0) ? (x/32768.0) : (x/32767.0);
}

inline Sint16 double_to_s16(double x){
	return Sint16((x<0) ? (x * 32768.0) : (x * 32767.0));
}

#define DEFAULT_BUFFER_SIZE 4096

class Mutex{
	SDL_mutex *mutex;
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();
};

Mutex::Mutex(){
	this->mutex = SDL_CreateMutex();
}

Mutex::~Mutex(){
	SDL_DestroyMutex(this->mutex);
}

void Mutex::lock(){
	SDL_LockMutex(this->mutex);
}

void Mutex::unlock(){
	SDL_UnlockMutex(this->mutex);
}

class RecursiveMutex{
	Mutex mutex, metamutex;
	unsigned lock_count;
	Uint32 locked_by;
public:
	RecursiveMutex(): lock_count(0), locked_by(0){}
	void lock();
	void unlock();
};

template <typename T>
class AutoLocker{
	T &lockable;
public:
	AutoLocker(T &l): lockable(l){
		this->lockable.lock();
	}
	~AutoLocker(){
		this->lockable.unlock();
	}
};

typedef AutoLocker<Mutex> AutoMutex;
typedef AutoLocker<RecursiveMutex> AutoRecursiveMutex;

void RecursiveMutex::lock(){
	Uint32 tid = SDL_ThreadID();
	{
		AutoMutex am(this->metamutex);
		if (this->lock_count && this->locked_by == tid){
			this->lock_count++;
			return;
		}
	}
	this->mutex.lock();
	this->lock_count++;
	this->locked_by = tid;
}

void RecursiveMutex::unlock(){
	{
		AutoMutex am(this->metamutex);
		this->lock_count--;
		if (this->lock_count)
			return;
	}
	this->mutex.unlock();
}

template <typename T>
class thread_safe_queue{
	std::queue<T> queue;
	Mutex mutex;
public:
	unsigned max_size;
	thread_safe_queue(){
		this->max_size = std::numeric_limits<unsigned>::max();
	}
	thread_safe_queue(const thread_safe_queue &b){
		AutoMutex am(b.mutex);
		this->queue = b.queue;
	}
	const thread_safe_queue &operator=(const thread_safe_queue &b){
		AutoMutex am[] = {
			b.mutex,
			this->mutex
		};
		this->queue = b.queue;
	}
#if 0
	void lock(){
		this->mutex.lock();
	}
	void unlock(){
		this->mutex.unlock();
	}
#endif
	void push(const T &e){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size < this->max_size){
					this->queue.push(e);
					return;
				}
			}
			SDL_Delay(10);
		}
		
	}
	bool is_empty(){
		AutoMutex am(this->mutex);
		return this->queue.empty();
	}
	size_t size(){
		AutoMutex am(this->mutex);
		return this->queue.size();
	}
	T &peek(){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size > 0)
					return this->queue.front();
			}
			SDL_Delay(10);
		}
	}
	T *try_peek(){
		AutoMutex am(this->mutex);
		if (!this->queue.size())
			return 0;
		return &this->queue.front();
	}
	T pop(){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size>0){
					T ret = this->queue.front();
					this->queue.pop();
					return ret;
				}
			}
			SDL_Delay(10);
		}
	}
	bool try_pop(T &o){
		AutoMutex am(this->mutex);
		if (!this->queue.size())
			return 0;
		o = this->queue.front();
		this->queue.pop();
		return 1;
	}
	void pop_without_copy(){
		while (1){
			{
				AutoMutex am(this->mutex);
				size_t size = this->queue.size();
				if (size > 0){
					this->queue.pop();
					return;
				}
			}
			SDL_Delay(10);
		}
	}
};

typedef Sint16 sample_t;
typedef Uint64 sample_count_t;
typedef Uint64 audio_position_t;

struct audio_buffer_t{
	sample_t *data;
	unsigned channel_count;
	unsigned sample_count;
	unsigned samples_produced;
	unsigned samples_consumed;
	audio_buffer_t(): data(0), channel_count(0), sample_count(0), samples_produced(0), samples_consumed(0){}
	sample_t *operator[](size_t i){
		return this->data + i * this->channel_count;
	}
	static audio_buffer_t alloc(unsigned channels, unsigned length){
		audio_buffer_t ret;
		ret.data = new sample_t[channels * length];
		ret.sample_count = length;
		ret.channel_count = channels;
		ret.samples_produced = 0;
		ret.samples_consumed = 0;
#ifdef _DEBUG
		memset(ret.data, 0xCD, sizeof(sample_t) * channels * length);
#endif
		return ret;
	}
	audio_buffer_t alloc(){
		audio_buffer_t ret;
		ret.data = new sample_t[this->channel_count * this->sample_count];
		ret.sample_count = this->sample_count;
		ret.channel_count = this->channel_count;
		ret.samples_produced = this->samples_produced;
		ret.samples_consumed = this->samples_consumed;
#ifdef _DEBUG
		memset(ret.data, 0xCD, sizeof(sample_t) * this->channel_count * this->sample_count);
#endif
		return ret;
	}
	void free(){
		delete[] this->data;
	}
};

class RingBufferQueue{
	audio_buffer_t buffers[2];
	audio_position_t position;
	char start;
	char size;
public:
	RingBufferQueue(): start(0), size(0), position(0){}
	~RingBufferQueue();
	void push(audio_buffer_t buffer);
	audio_buffer_t &operator[](char i);
	char get_size(){
		return this->size;
	}
	const sample_t *get_sample(audio_position_t, bool &below);
	unsigned copy_buffer(audio_buffer_t &buffer, audio_position_t &sample, bool &below);
};

RingBufferQueue::~RingBufferQueue(){
	for (char i = 0; i < this->size; i++)
		(*this)[i].free();
}

void RingBufferQueue::push(audio_buffer_t buffer){
	switch (this->size){
		case 0:
			this->buffers[this->start & 1]=buffer;
			this->size++;
			break;
		case 1:
			this->buffers[(this->start + 1) & 1]=buffer;
			this->size++;
			break;
		case 2:
			this->position += this->buffers[this->start].sample_count;
			this->buffers[this->start].free();
			this->buffers[this->start] = buffer;
			this->start = (this->start+1) & 1;
	}
}

audio_buffer_t &RingBufferQueue::operator[](char i){
	return this->buffers[(this->start + i) & 1];
}

const sample_t *RingBufferQueue::get_sample(audio_position_t p, bool &below){
	below = p<this->position;
	if (below)
		return 0;
	size_t diff = size_t(p - this->position);
	switch (this->size){
		case 0:
			return 0;
		case 1:
			if ((*this)[0].sample_count <= diff)
				return 0;
			return (*this)[0][diff];
		case 2:
			if ((*this)[0].sample_count > diff)
				return (*this)[0][diff];
			if ((*this)[1].sample_count <= diff)
				return 0;
			return (*this)[1][diff];
	}
	assert(0);
	return 0;
}

unsigned RingBufferQueue::copy_buffer(audio_buffer_t &buffer, audio_position_t &p, bool &below){
	below = p < this->position;
	if (below)
		return 0;
	unsigned diff = unsigned(p - this->position);
	unsigned samples_to_copy;
	unsigned bytes_to_copy;
	switch (this->size){
		case 0:
			return 0;
		case 1:
			if ((*this)[0].sample_count <= diff)
				return 0;
			samples_to_copy = std::min(buffer.sample_count, (*this)[0].sample_count - diff);
			bytes_to_copy = samples_to_copy * buffer.channel_count * 2;
			memcpy(buffer.data, (*this)[0][diff], bytes_to_copy);
			return samples_to_copy;
		case 2:
			if ((*this)[0].sample_count>diff){
				samples_to_copy = std::min(buffer.sample_count, (*this)[0].sample_count - diff);
				bytes_to_copy = samples_to_copy * buffer.channel_count * 2;
				memcpy(buffer.data, (*this)[0][diff], bytes_to_copy);
				return samples_to_copy;
			}
			diff -= (*this)[0].sample_count;
			if ((*this)[1].sample_count <= diff)
				return 0;
			samples_to_copy = std::min(buffer.sample_count, (*this)[1].sample_count - diff);
			bytes_to_copy = samples_to_copy * buffer.channel_count * 2;
			memcpy(buffer.data, (*this)[1][diff], bytes_to_copy);
			return samples_to_copy;
	}
	assert(0);
	return 0;
}

class Decoder{
protected:
	RingBufferQueue buffers;

	bool offset_operator_helper(const sample_t *&ret, audio_position_t sample, int index);
	bool direct_output_helper(sample_count_t &ret, audio_buffer_t &buffer, audio_position_t &initial_position, int index);
	virtual audio_buffer_t read_more() = 0;
	bool read_next();
public:
	virtual ~Decoder(){}
	virtual unsigned get_sampling_rate() = 0;
	const sample_t *operator[](audio_position_t);
	sample_count_t direct_output(audio_buffer_t buffer, audio_position_t initial_position);
};

const sample_t *Decoder::operator[](audio_position_t position){
	while (1){
		bool below;
		const sample_t *sample = this->buffers.get_sample(position, below);
		if (!sample){
			if (below)
				return 0;
			if (!this->read_next())
				return 0;
			continue;
		}
		return sample;
	}
}

sample_count_t Decoder::direct_output(audio_buffer_t buffer, audio_position_t sample){
	sample_count_t samples_read = 0;
	while (buffer.sample_count){
		bool below;
		unsigned call_result = this->buffers.copy_buffer(buffer, sample, below);
		if (!call_result){
			if (below)
				return 0;
			if (!this->read_next())
				break;
			continue;
		}
		samples_read += call_result;
		buffer.sample_count -= call_result;
		sample += call_result;
		buffer.data = buffer[call_result];
	}
	return samples_read;
}

audio_buffer_t remix_channels(audio_buffer_t buffer){
	switch (buffer.channel_count){
		case 1:
			{
				audio_buffer_t temp = audio_buffer_t::alloc(2, buffer.sample_count);
				temp.samples_produced = buffer.samples_produced;
				for (sample_count_t i = 0; i < temp.sample_count; i++){
					temp[(size_t)i][1] =
					temp[(size_t)i][0] = buffer[(size_t)i][0];
				}
				buffer.free();
				buffer = temp;
			}
			break;
		case 2:
			break;
		default:
			{
				double multiplier = 1.0 / double(buffer.channel_count);
				audio_buffer_t temp = audio_buffer_t::alloc(2, buffer.sample_count);
				temp.samples_produced = buffer.samples_produced;
				for (sample_count_t i = 0; i < temp.sample_count; i++){
					double avg = 0;
					sample_t *sample = buffer[(size_t)i];
					for (unsigned channel = 0; channel < buffer.channel_count; channel++)
						avg += s16_to_double(sample[channel]);
					avg *= multiplier;
					temp[(size_t)i][1] =
					temp[(size_t)i][0] = double_to_s16(avg);
				}
				buffer.free();
				buffer=temp;
			}
			break;
	}
	return buffer;
}

bool Decoder::read_next(){
	audio_buffer_t buffer = this->read_more();
	if (!buffer.data)
		return 0;
	
	this->buffers.push(remix_channels(buffer));
	return 1;
}

const char *ogg_code_to_string(int e){
	switch (e){
		case 0:
			return "no error";
		case OV_EREAD:
			return "a read from media returned an error";
		case OV_ENOTVORBIS:
			return "bitstream does not contain any Vorbis data";
		case OV_EVERSION:
			return "vorbis version mismatch";
		case OV_EBADHEADER:
			return "invalid Vorbis bitstream header";
		case OV_EFAULT:
			return "internal logic fault; indicates a bug or heap/stack corruption";
		default:
			return "unknown error";
	}
}

class OggDecoder: public Decoder{
	FILE *file;
	int bitstream;
	OggVorbis_File ogg_file;
	unsigned frequency;
	unsigned channels;

	audio_buffer_t read_more(){
		const size_t samples_to_read = 1024;
		const size_t bytes_per_sample = this->channels*2;
		const size_t bytes_to_read = samples_to_read*bytes_per_sample;
		audio_buffer_t ret = audio_buffer_t::alloc(this->channels, samples_to_read);
		size_t size = 0;
		while (size < bytes_to_read){
			int r = ov_read(&this->ogg_file, ((char *)ret.data) + size, int(bytes_to_read - size), 0, 2, 1, &this->bitstream);
			if (r < 0)
				abort();
			if (!r){
				if (!size){
					ret.free();
					ret.data = 0;
					ret.sample_count = 0;
				}
				break;
			}
			size += r;
		}
		ret.sample_count = ret.samples_produced = unsigned(size / bytes_per_sample);
		return ret;
	}
public:
	OggDecoder(const char *filename){
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
		this->file = fopen(filename, "rb");
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		if (!this->file)
			abort();
		this->bitstream = 0;
		ov_callbacks cb;
		cb.read_func = OggDecoder::read;
		cb.seek_func = OggDecoder::seek;
		cb.tell_func = OggDecoder::tell;
		cb.close_func = 0;
		int error = ov_open_callbacks(this, &this->ogg_file, 0, 0, cb);
		if (error < 0)
			abort();
		vorbis_info *i = ov_info(&this->ogg_file, this->bitstream);
		this->frequency = i->rate;
		this->channels = i->channels;
	}
	~OggDecoder(){
		if (this->file)
			fclose(this->file);
		ov_clear(&this->ogg_file);
	}
	unsigned get_sampling_rate(){
		return this->frequency;
	}

	static size_t read(void *buffer, size_t size, size_t nmemb, void *s){
		OggDecoder *_this = (OggDecoder *)s;
		size_t ret = fread(buffer, size, nmemb, _this->file);
		return ret;
	}
	static int seek(void *s, ogg_int64_t offset, int whence){
		OggDecoder *_this = (OggDecoder *)s;
		int ret = fseek(_this->file, (long)offset, whence);
		return ret;
	}
	static long tell(void *s){
		OggDecoder *_this = (OggDecoder *)s;
		long ret = ftell(_this->file);
		return ret;
	}
};


class ResamplingFilter{
	enum Mode{
		NOOP,
		DOWNSAMPLING,
		UPSAMPLING,
	} mode;
	Decoder &decoder;
	unsigned src_rate;
	unsigned dst_rate;
public:
	ResamplingFilter(Decoder &decoder, unsigned d);
	sample_count_t read(audio_buffer_t buffer, audio_position_t position);
};

ResamplingFilter::ResamplingFilter(Decoder &decoder, unsigned d): decoder(decoder), dst_rate(d){
	this->src_rate = decoder.get_sampling_rate();
	if (this->src_rate == this->dst_rate)
		this->mode = NOOP;
	else if (this->src_rate < this->dst_rate)
		this->mode = UPSAMPLING;
	else if (this->src_rate > this->dst_rate)
		this->mode = DOWNSAMPLING;
}

sample_count_t ResamplingFilter::read(audio_buffer_t buffer, audio_position_t position){
	switch (this->mode){
		case NOOP:
			return this->decoder.direct_output(buffer, position);
		case UPSAMPLING:
			//Upsampling performed by nearest neighbor. (Sort of. The position value gets truncated, not rounded.)
			{
				sample_count_t samples_read = 0;
				for (unsigned i = 0; i < buffer.sample_count; i++){
					//This multiplication:
					//    position*this->src_rate
					//would need, pessimistically, a position value greater than 15 years in order to overflow.
					//(2^64-1)/192000/192000/86400/365.2425 ~= 15.86
					audio_position_t src_sample = position * this->src_rate / this->dst_rate;
					const sample_t *sample = this->decoder[src_sample];
					if (!sample)
						break;
					for (unsigned j = 0; j < buffer.channel_count; j++)
						*(buffer.data++) = *(sample++);
					samples_read++;
				}
				return samples_read;
			}
		case DOWNSAMPLING:
			{
				sample_count_t samples_read = 0;
				for (unsigned i = 0; i < buffer.sample_count; i++){
					double fraction = double(this->src_rate) / double(this->dst_rate);
					double src_sample0 = position*fraction;
					double src_sample1 = (position + 1) * fraction;
					double accumulators[256] = {0};
					double count=0;
					bool end=0;
					for (double j = src_sample0; j < src_sample1;){
						double next = floor(j + 1);
						const sample_t *sample = this->decoder[(audio_position_t)j];
						if (!sample){
							end = 1;
							break;
						}
						double limit = (next < src_sample1) ? next : src_sample1;
						for (unsigned channel = 0; channel < buffer.channel_count; channel++){
							double value = s16_to_double(sample[channel]);
							accumulators[channel] += (limit - j) * value;
						}
						count += limit - j;
						j = next;
					}
					if (count){
						for (unsigned channel = 0; channel < buffer.channel_count; channel++)
							*(buffer.data++) = double_to_s16(accumulators[channel] / count);
						samples_read++;
					}
					if (end)
						break;
				}
				return samples_read;
			}
	}
	return 0;
}

Decoder *make_decoder(const char *filename){
	return new OggDecoder(filename);
}

class AudioStream{
	Decoder *decoder;
	ResamplingFilter *filter;
	unsigned frequency;
	unsigned channels;
	unsigned default_buffer_length;
	audio_position_t position;
public:
	AudioStream(const char *filename, unsigned frequency, unsigned channels, unsigned dfl);
	~AudioStream();
	audio_buffer_t read_new();
	void reset();
};

AudioStream::AudioStream(const char *filename, unsigned frequency, unsigned channels, unsigned dfl){
	this->frequency = frequency;
	this->channels = channels;
	this->default_buffer_length = dfl;
	this->position = 0;
	this->decoder = make_decoder(filename);
	this->filter = 0;
	if (!this->decoder)
		return;
	this->filter = new ResamplingFilter(*this->decoder, frequency);
}

AudioStream::~AudioStream(){
	delete this->filter;
	delete this->decoder;
}

audio_buffer_t AudioStream::read_new(){
	audio_buffer_t ret;
	if (!this->filter)
		return ret;
	ret = audio_buffer_t::alloc(this->channels, this->default_buffer_length);
	sample_count_t samples_read = this->filter->read(ret, this->position);
	if (!samples_read){
		ret.free();
		ret.data = 0;
	}else{
		this->position += samples_read;
		ret.samples_produced += (unsigned)samples_read;
	}
	return ret;
}

class AudioPlayer{
	friend void AudioCallback(void *udata, Uint8 *stream, int len);
	friend int main(int argc, char **argv);
	thread_safe_queue<audio_buffer_t> queue;
	SDL_Thread *sdl_thread;
	AudioStream *now_playing;
	std::queue<const char *> playlist;
	volatile bool run;
	static int _thread(void *);
	void thread();
public:
	AudioPlayer();
	~AudioPlayer();
	void test();
};

AudioPlayer::AudioPlayer(){
#ifdef WIN32
	//Put your test tracks here when compiling for Windows.
	//TODO: Other systems.
#else
	//Put your test tracks here when compiling for Android.
#endif
	this->queue.max_size = 100;
	this->now_playing = 0;
	this->run = 1;
	this->sdl_thread = SDL_CreateThread(_thread, "AudioPlayerThread", this);
}

AudioPlayer::~AudioPlayer(){
	this->run = 0;
	//SDL_WaitThread(this->sdl_thread, 0);
	while (!this->queue.is_empty())
		this->queue.pop().free();
}

int AudioPlayer::_thread(void *p){
	((AudioPlayer *)p)->thread();
	return 0;
}

Clock::t cpu_time = 0,
	playback_time = 0;

void AudioPlayer::thread(){
	Clock c;
	unsigned long long count = 0;
	while (this->run){
#if 0
		{
			unsigned fullness = unsigned(this->queue.size() * 40 / this->queue.max_size);
			//std::cerr <<"queue.size() = [";
			char display[41];
			display[40] = 0;
			for (unsigned i = 0; i < 40; i++)
				display[i] = (i < fullness) ? '#' : ' ';
			std::cerr <<display<<"]\n";
		} 
#endif

		if (!this->now_playing){
			if (!this->playlist.size()){
				SDL_Delay(50);
				continue;
			}
			const char *filename = this->playlist.front();
			//std::cerr <<"now playing "<<this->playlist.front()<<std::endl;
			this->now_playing = new AudioStream(filename, 44100, 2, DEFAULT_BUFFER_SIZE);
			this->playlist.pop();
		}
		Clock::t t0 = c();
		audio_buffer_t buffer = this->now_playing->read_new();
		Clock::t t1 = c();
		cpu_time = t1 - t0;
		playback_time = double(buffer.sample_count) / (44.1 * 2.0);
		//std::cerr << cpu_time / playback_time * 100 <<" %\t"<<playback_time / cpu_time<<std::endl;
		if (!buffer.data){
			delete this->now_playing;
			//this->now_playing = new AudioStream("f:/Data/Music/Judas Priest/test.ogg", 44100, 2, DEFAULT_BUFFER_SIZE);
			this->now_playing = 0;
			continue;
		}
		count++;
		this->queue.push(buffer);
	}
}

void AudioPlayer::test(){
	unsigned long long count = 0;
	while (this->run){
		if (!this->now_playing){
			if (!this->playlist.size())
				break;
			const char *filename = this->playlist.front();
			//std::cerr <<"now playing "<<this->playlist.front()<<std::endl;
			this->now_playing = new AudioStream(filename, 44100, 2, DEFAULT_BUFFER_SIZE);
			this->playlist.pop();
		}
		audio_buffer_t buffer = this->now_playing->read_new();
		if (!buffer.data){
			delete this->now_playing;
			this->now_playing = 0;
			continue;
		}else
			buffer.free();
		count++;
	}
}

//#define PCM_TO_FILE

#ifdef PCM_TO_FILE
std::ofstream ofile("output.raw", std::ios::binary|std::ios::trunc);
#endif

double t = 0;

void AudioCallback(void *udata, Uint8 *stream, int len){
	/*
	{
		len /= 4;
		for (; len; len--, stream += 4, t += 1.0/44100.0){
			Sint16 *p = (Sint16 *)stream;
			double amplitude = sin(t * (3.141592 * 1000));
			p[1] = p[0] = amplitude * 32767;
		}
		return;
	}
	*/
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
#ifdef PCM_TO_FILE
			ofile.write((const char *)stream, len);
#endif
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
#ifdef PCM_TO_FILE
	ofile.write((const char *)stream, len);
#endif
}

int main(int argc, char **argv){
	//const size_t N=1<<24;
	//char *raw_buffer=new char[N];
	Clock c;
	Clock::t t0 = c();
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
