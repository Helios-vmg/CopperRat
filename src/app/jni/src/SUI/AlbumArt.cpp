/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "../stdafx.h"

#include "MainScreen.h"
#include "SUI.h"
#include "../File.h"
#include "../Image.h"
#include "../CommonFunctions.h"
#include "../Metadata.h"

#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <memory>
#endif

extern const char * const vertex_shader =
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

static std::string generate_fragment_shader(double sigma, double texture_w, double texture_h, bool vertical){
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
	this->apply_blur = true;
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

//static std::wstring get_blurred_image_path(const std::wstring &path){
//	return get_blurred_image_path_from_hash(get_hash(path));
//}

void MainScreen::load_image(GenericMetadata &metadata, const std::wstring &original_source){
	struct Loader{
		GenericMetadata &metadata;
		const std::wstring &original_source;
		std::wstring new_source;
		std::wstring cached_source;
		bool skip_loading = false;
		bool skip_resize = false;
		surface_t picture;
		std::string hash;
		unsigned target_square;

		Loader(MainScreen &screen, GenericMetadata &metadata, const std::wstring &original_source): metadata(metadata), original_source(original_source){
			this->target_square = screen.get_ui().get_bounding_square();
			this->new_source = original_source;
		}
		void operator()(){
			unsigned char *buffer;
			size_t length;
			if (this->metadata.picture(buffer, length)){
				this->new_source = this->metadata.get_path();
				this->load_picture_from_cache(this->new_source);
				if (!this->picture)
					this->picture = load_image_from_memory(buffer, length);
			}else{
				this->load_picture_from_filesystem();
			}
			if (this->picture && !this->skip_resize){
				this->picture = bind_surface_to_square(this->picture, this->target_square);
				save_surface_compressed(string_to_utf8(get_cached_image_path(this->new_source)).c_str(), this->picture);
			}
		}

	private:
		void load_picture_from_cache(const std::wstring &s){
			this->hash = ::get_hash(s);
			auto hashed_path = get_cached_image_path(s);
			this->picture = load_image_from_file(hashed_path);
			if (!this->picture)
				return;
			this->skip_resize = true;
			this->cached_source = std::move(hashed_path);
		}
		void load_picture_from_filesystem(){
			auto path = this->metadata.get_path();
			auto directory = get_contaning_directory(path);

			std::wstring patterns[5];

			{
				patterns[0] = L"front.*";
				patterns[1] = L"cover.*";
				{
					patterns[2] = path;
					auto last_dot = patterns[2].rfind('.');
					patterns[2] = patterns[2].substr(0, last_dot);
					patterns[2] += L".*";
				}
				{
					patterns[3] = this->metadata.album();
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
						if (path == this->original_source){
							this->skip_loading = true;
							return;
						}
						this->load_picture_from_cache(path);
						if (!this->picture)
							this->picture = load_image_from_file(path);
						if (this->picture){
							this->new_source = path;
							return;
						}
					}
				}
			}
			this->skip_loading = false;
		}
	};
	
	Loader loader(*this, metadata, original_source);
	loader();
	//We're done, so hand-off the results to the main thread so it can decide what to do.
	this->sui->push_async_callback([
		this,
		p = std::move(loader.picture),
		s = std::move(loader.new_source),
		h = std::move(loader.hash),
		sl = loader.skip_loading
	](){
		this->finish_picture_load(p, s, h, sl);
	});
}

void MainScreen::blur_image(surface_t image, const std::wstring &path){
	auto max_square = this->sui->get_max_square();
	auto visible_region = this->sui->get_visible_region();
	this->sui->push_parallel_work([this, image0 = image, path, max_square, visible_region](){
		auto image = image0;
		auto t0 = clock();
		{
			if (image->w > image->h)
				image = scale_surface(image, max_square * image->w / image->h, max_square);
			else if (image->w < image->h)
				image = scale_surface(image, max_square, max_square * image->h / image->w);
			else
				image = scale_surface(image, max_square, max_square);
			auto temp = create_rgbq_surface(image->format->BitsPerPixel, visible_region.w, visible_region.h);
			SDL_Rect src_rect = {
				-(visible_region.w - image->w) / 2,
				-(visible_region.h - image->h) / 2,
				visible_region.w,
				visible_region.h,
			};
			SDL_BlitSurface(image.get(), &src_rect, temp.get(), nullptr);
			image = temp;
			image = apply_gaussian_blur2(image, 15);
			save_surface_compressed(string_to_utf8(path).c_str(), image, 100);
		}
		auto t1 = clock();
		double t = double(t1 - t0);
		t /= (double)CLOCKS_PER_SEC;
		t *= 1000;
		__android_log_print(ANDROID_LOG_INFO, "C++Blur", "MainScreen::blur_image(): %f\n", t);

		this->sui->push_async_callback([this, image](){
			this->finish_background_load(image);
		});
	});
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
