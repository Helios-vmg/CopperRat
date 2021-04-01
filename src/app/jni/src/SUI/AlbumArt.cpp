/*

Copyright (c) 2014, Helios
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "../stdafx.h"
#include "SUI.h"
#include "../File.h"
#include "../Image.h"

#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <memory>
#endif

class DelayedPictureLoadStartAction : public DelayedPictureLoadAction{
	SUI *sui;
	std::shared_ptr<PictureDecodingJob> job;
public:
	DelayedPictureLoadStartAction(SUI &sui, std::shared_ptr<PictureDecodingJob> job): sui(&sui), job(job){}
	void perform(){
		this->sui->start_picture_load(this->job);
	}
};

class DelayedTextureLoadAction : public DelayedPictureLoadAction{
	SUI *sui;
	surface_t picture;
	std::wstring source_path;
	std::string hash;
	bool skip_loading;
public:
	DelayedTextureLoadAction(SUI &sui, PictureDecodingJob &job):
		sui(&sui),
		picture(job.get_picture()),
		source_path(job.get_source()),
		hash(job.get_hash()),
		skip_loading(job.get_skip_loading()) {}
	void perform(){
		this->sui->finish_picture_load(this->picture, this->source_path, this->hash, this->skip_loading);
	}
};

class DelayedBGTextureLoadAction : public DelayedPictureLoadAction{
	SUI *sui;
	surface_t picture;
public:
	DelayedBGTextureLoadAction(SUI &sui, PictureBlurringJob &job):
		sui(&sui),
		picture(job.get_picture()) {}
	void perform(){
		this->sui->finish_background_load(this->picture);
	}
};

const char *vertex_shader =
#ifndef __ANDROID__
"#version 120\n"
#else
"#version 100\n"
"precision mediump int;\n"
"precision mediump float;\n"
#endif
"\n"
"attribute vec3 gpu_Vertex;\n"
"attribute vec2 gpu_TexCoord;\n"
"attribute vec4 gpu_Color;\n"
"uniform mat4 modelViewProjection;\n"
"\n"
"varying vec4 color;\n"
"varying vec2 texCoord;\n"
"\n"
"void main(void)\n"
"{\n"
"	color = gpu_Color;\n"
"	texCoord = vec2(gpu_TexCoord);\n"
"	gl_Position = modelViewProjection * vec4(gpu_Vertex, 1.0);\n"
"}\n";


double gauss_kernel(double x, double sigma){
	return exp((x * x) / -(2 * sigma * sigma)) / (9.4247779607693797153879301498385 * sigma * sigma);
}

std::string generate_fragment_shader(double sigma, double texture_w, double texture_h, bool vertical){
	std::stringstream stream;
	stream <<
#ifndef __ANDROID__
"#version 120\n"
#else
"#version 100\n"
"precision mediump int;\n"
"precision mediump float;\n"
#endif
"\n"
"varying vec4 color;\n"
"//varying vec2 v_texCoord;\n"
"varying vec2 texCoord;\n"
"\n"
"uniform sampler2D tex;\n"
"uniform float time;\n"
"\n"
"void main(void)\n"
"{\n"
"	vec4 accum = vec4(0, 0, 0, 0);\n";
	double normalization = 0;
	std::vector<double> xs;
	std::vector<double> kernels;
	for (double x = -sigma * 2; x <= sigma * 2; x++){
		double kernel = gauss_kernel(x, sigma);
		normalization += kernel;
		kernels.push_back(kernel);
		xs.push_back(x);
	}
	for (size_t i = 0; i < kernels.size(); i++){
		double kernel = kernels[i] / normalization;
		//if (kernel < 1.0/256.0)
		//	continue;
		double x = xs[i];
		stream <<"	accum += texture2D(tex, texCoord + vec2(";
		if (!vertical)
			stream <<(x / texture_w)<<", 0";
		else
			stream <<"0, "<<(x / texture_h);
		stream <<")) * "<<kernel<<";\n";
	}
	stream <<"	gl_FragColor = accum;\n}\n";
	return stream.str();
}

void SUI::create_shaders(){
	auto rect = this->get_visible_region();
	auto vertex = Shader::create(vertex_shader, 0);
	if (!*vertex){
		__android_log_print(ANDROID_LOG_ERROR, "C++Shader", "%s", vertex->get_error_string().c_str());
		return;
	}
	auto frag1 = Shader::create(generate_fragment_shader(15, rect.w, rect.h, 0).c_str());
	if (!*frag1){
		__android_log_print(ANDROID_LOG_ERROR, "C++Shader", "%s", frag1->get_error_string().c_str());
		return;
	}
	auto frag2 = Shader::create(generate_fragment_shader(15, rect.w, rect.h, 1).c_str());
	if (!*frag2){
		__android_log_print(ANDROID_LOG_ERROR, "C++Shader", "%s", frag2->get_error_string().c_str());
		return;
	}
	this->blur_h.add(vertex);
	this->blur_h.add(frag1);
	this->blur_h.create_internal_object();
	if (!blur_h){
		__android_log_print(ANDROID_LOG_ERROR, "C++Shader", "%s", this->blur_h.get_error_string().c_str());
		return;
	}
	this->blur_v.add(vertex);
	this->blur_v.add(frag2);
	this->blur_v.create_internal_object();
	if (!blur_v){
		__android_log_print(ANDROID_LOG_ERROR, "C++Shader", "%s", this->blur_v.get_error_string().c_str());
		return;
	}
	this->apply_blur = 1;
}

template <typename Iterator>
size_t generalized_hash(Iterator begin, const Iterator &end){
	assert(sizeof(size_t) == 4 || sizeof(size_t) == 8);
	size_t ret = 0xF00BA8;
	const size_t factor = sizeof(size_t) != 8 ? 0xDEADBEEF : 0x8BADF00DDEADBEEF;
	for (; begin != end; ++begin){
		ret *= factor;
		ret ^= *begin;
	}
	return ret;
}

size_t hash_string(const std::string &s){
	return generalized_hash(s.begin(), s.end());
}

static std::string get_hash(const std::wstring &s){
	const char *digits = "0123456789ABCDEF";
	auto string_to_hash = string_to_utf8(s);
	size_t digest = hash_string(string_to_hash),
		copy = digest;
	char hash[sizeof(digest) * 2 + 1];
	for (int i = 0; i < sizeof(digest) * 2; i++){
		hash[sizeof(digest) * 2 - 1 - i] = digits[copy & 0x0F];
		copy >>= 4;
	}
	hash[sizeof(digest) * 2] = 0;
	return hash;
}

static std::wstring get_cached_image_path(const std::wstring &path){
	return utf8_to_string(std::string(BASE_PATH) + get_hash(path) + ".webp");
}

static std::wstring get_blurred_image_path_from_hash(const std::string &hash){
	return utf8_to_string(std::string(BASE_PATH) + hash + "-blur.webp");
}

static std::wstring get_blurred_image_path(const std::wstring &path){
	return get_blurred_image_path_from_hash(get_hash(path));
}

void PictureDecodingJob::sui_perform(WorkerThread &wt){
	this->skip_resize = 0;
	unsigned char *buffer;
	size_t length;
	std::wstring string_to_hash;
	if (this->metadata->picture(buffer, length)){
		this->source = this->metadata->get_path();
		if (!this->load_picture_from_cache(this->source))
			this->picture = load_image_from_memory(buffer, length);
	}else{
		this->load_picture_from_filesystem();
	}
	if (this->picture && !this->skip_resize){
		this->picture = bind_surface_to_square(this->picture, this->target_square);
		save_surface_compressed(string_to_utf8(get_cached_image_path(this->source)).c_str(), this->picture);
	}
}

bool PictureDecodingJob::load_picture_from_cache(const std::wstring &s){
	this->hash = ::get_hash(s);
	auto hashed_path = get_cached_image_path(s);
	this->picture = load_image_from_file(hashed_path);
	if (this->picture){
		this->skip_resize = 1;
		this->cached_source = hashed_path;
		return 1;
	}
	return 0;
}

void PictureDecodingJob::load_picture_from_filesystem(){
	auto path = this->metadata->get_path();
	auto directory = get_contaning_directory(path);
	auto path_file = get_filename(path);

	std::wstring patterns[5];

	{
		patterns[0] = L"front.*";
		patterns[1] = L"cover.*";
		{
			patterns[2] = path_file;
			auto last_dot = patterns[2].rfind('.');
			patterns[2] = patterns[2].substr(0, last_dot);
			patterns[2] += L".*";
		}
		{
			patterns[3] = this->metadata->album();
			patterns[3] += L".*";
		}
		patterns[4] = L"folder.*";
	}

	std::vector<DirectoryElement> files;
	list_files(files, directory, FilteringType::RETURN_FILES);

	for (auto &pattern : patterns){
		for (auto &element : files){
			const auto &filename = element.name;
			if (glob(pattern.c_str(), filename.c_str(), [](wchar_t c){ return tolower(c == '\\' ? '/' : c); })){
				auto path = directory;
				path += L"/";
				path += filename;
				if (path == this->current_source){
					skip_loading = 1;
					return;
				}
				if (!this->load_picture_from_cache(path))
					this->picture = load_image_from_file(path);
				if (this->picture){
					this->source = path;
					return;
				}
			}
		}
	}
	this->skip_loading = 0;
}

unsigned PictureDecodingJob::finish(SUI &sui){
	return sui.finish(*this);
}

void SUI::start_picture_load(std::shared_ptr<PictureDecodingJob> job){
	this->picture_job = this->worker.attach(job);
}

void SUI::start_picture_blurring(std::shared_ptr<PictureBlurringJob> job){
	this->picture_job = this->worker.attach(job);
}

unsigned SUI::receive(MetaDataUpdate &mdu){
	auto metadata = mdu.get_metadata();
	this->metadata.clear();
	auto track_number = metadata->track_number();
	auto track_artist = metadata->track_artist();
	auto track_title = metadata->track_title();
	auto size = this->metadata.size();
	this->metadata += track_number;
	if (size < this->metadata.size())
		this->metadata += L" - ";
	size = this->metadata.size();
	this->metadata += track_artist;
	if (size < this->metadata.size())
		this->metadata += L" - ";
	this->metadata += track_title;

	if (!this->metadata.size())
		this->metadata = get_filename(metadata->get_path());

	std::shared_ptr<PictureDecodingJob> job(new PictureDecodingJob(
		this->finished_jobs_queue, metadata,
		this->get_bounding_square(),
		this->get_visible_region(),
		this->tex_picture_source
	));
	job->description = to_string(metadata->get_path());
	if (this->ui_in_foreground)
		this->start_picture_load(job);
	else{
		this->dpla.reset(new DelayedPictureLoadStartAction(*this, job));
	}
	return NOTHING;
}

Texture SUI::blur_image(Texture tex){
	GPU_Target *screen = this->screen;
	Texture ret(screen);
	if (!this->apply_blur)
		return ret;
	clock_t t0 = 0, t1 = 0;

	t0 = clock();

	auto w = screen->w;
	auto h = screen->h;
	
	RenderTarget target1(w, h);
	RenderTarget target2(w, h);
	
	GPU_Clear(target1.get_target());
	tex.draw_with_fill2(target1.get_target());
	GPU_FlushBlitBuffer();

	this->blur_h.activate();
	
	GPU_Clear(target2.get_target());
	GPU_Blit(target1.get_target()->image, nullptr, target2.get_target(), w / 2, h / 2);
	GPU_FlushBlitBuffer();

	this->blur_v.activate();
	
	GPU_Clear(target1.get_target());
	GPU_Blit(target2.get_target()->image, nullptr, target1.get_target(), w / 2, h / 2);
	GPU_FlushBlitBuffer();

	GPU_ActivateShaderProgram(0, nullptr);

	ret = target1.get_image();
	t1 = clock();
	
	__android_log_print(ANDROID_LOG_INFO, "C++Shader", "Blurring done in %f ms\n", (double)(t1 - t0) / (double)CLOCKS_PER_SEC * 1000.0);
	return ret;
}

unsigned SUI::finish(PictureDecodingJob &job){
	unsigned ret = NOTHING;
	if (job.get_id() != this->picture_job->get_id())
		return ret;
	auto picture = job.get_picture();
	this->picture_job.reset();
	if (this->ui_in_foreground){
		ret = this->finish_picture_load(picture, job.get_source(), job.get_hash(), job.get_skip_loading());
	}else{
		this->dpla.reset(new DelayedTextureLoadAction(*this, job));
	}
	return ret;
}

unsigned SUI::finish_picture_load(surface_t picture, const std::wstring &source, const std::string &hash, bool skip_loading){
	if (skip_loading){
		__android_log_print(ANDROID_LOG_INFO, "C++AlbumArt", "%s", "Album art load optimized away.\n");
		return NOTHING;
	}

	if (!picture){
		this->tex_picture_source.clear();
		this->tex_picture.unload();
		this->background_picture.unload();
		return NOTHING;
	}

	this->tex_picture_source = source;
	this->tex_picture.load(picture);
	{
		//Try to load cached blurred background image.
		auto bg_path = get_blurred_image_path_from_hash(hash);
		bool loaded = 0;
		if (file_exists(bg_path)){
			auto surface = load_image_from_file(bg_path);
			if (surface){
				loaded = 1;
				this->background_picture.load(surface);
			}
		}
		if (!loaded){
			this->start_picture_blurring(std::make_shared<PictureBlurringJob>(
				this->finished_jobs_queue,
				this->get_max_square(),
				this->get_visible_region(),
				picture,
				bg_path
			));
			this->background_picture = this->blur_image(this->tex_picture);
		}
	}

	return REDRAW;
}

void PictureBlurringJob::sui_perform(WorkerThread &wt){
	auto t0 = clock();
	{
		if (this->picture->w > this->picture->h)
			this->picture = scale_surface(this->picture, this->target_square * this->picture->w / this->picture->h, this->target_square);
		else if (this->picture->w < this->picture->h)
			this->picture = scale_surface(this->picture, this->target_square, this->target_square * this->picture->h / this->picture->w);
		else
			this->picture = scale_surface(this->picture, this->target_square, this->target_square);
		auto temp = create_rgbq_surface(this->picture->format->BitsPerPixel, this->trim_rect.w, this->trim_rect.h);
		SDL_Rect src_rect = {
			-(this->trim_rect.w - this->picture->w) / 2,
			-(this->trim_rect.h - this->picture->h) / 2,
			this->trim_rect.w,
			this->trim_rect.h,
		};
		SDL_BlitSurface(this->picture.get(), &src_rect, temp.get(), nullptr);
		this->picture = temp;
		this->picture = apply_gaussian_blur2(this->picture, 15);
		save_surface_compressed(string_to_utf8(this->path).c_str(), this->picture, 100);
	}
	auto t1 = clock();
	double t = double(t1 - t0);
	t /= (double)CLOCKS_PER_SEC;
	t *= 1000;
	__android_log_print(ANDROID_LOG_INFO, "C++Blur", "PictureBlurringJob::sui_perform(): %f\n", t);
}

unsigned PictureBlurringJob::finish(SUI &sui){
	return sui.finish(*this);
}

unsigned SUI::finish(PictureBlurringJob &job){
	unsigned ret = NOTHING;
	if (job.get_id() != this->picture_job->get_id())
		return ret;
	auto picture = job.get_picture();
	this->picture_job.reset();
	if (this->ui_in_foreground){
		ret = this->finish_background_load(job.get_picture());
	}else{
		this->dpla.reset(new DelayedBGTextureLoadAction(*this, job));
	}
	return ret;
}

unsigned SUI::finish_background_load(surface_t picture){
	this->background_picture.load(picture);
	return REDRAW;
}
