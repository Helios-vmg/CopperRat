#include "../stdafx.h"
#include "MainScreen.h"
#include "../CommonFunctions.h"
#include "Button.h"
#include "../File.h"

enum class ButtonSignal{
	PLAY = 0,
	PAUSE,
	STOP,
	LOAD,
	PREVIOUS,
	SEEKBACK,
	SEEKFORTH,
	NEXT,
};

MainScreen::MainScreen(SUI *sui, GUIElement *parent, AudioPlayer &player):
		GUIElement(sui, parent),
		player(player){
	this->prepare_buttons();
}

const wchar_t *test_list[] = {
	L"01 - Fear on the Bridge.ogg",
	L"02 - Deadly Sinners.ogg",
	L"03 - Revenge is a Vulture.ogg",
	L"04 - Dominion of Deceit.ogg",
	L"05 - Premonition of Pain.ogg",
	L"06 - Lord of the Storm.ogg",
	L"07 - Wykydtron.ogg",
	L"08 - Swordmaster.ogg",
	L"09 - Axes of Evil.ogg",
	L"10 - Crazy Nights.ogg",
	L"11 - Destroy the Orcs.ogg",
	L"12 - The Phantom of the Crimson Cloak.ogg",
	L"13 - Isle of Eternal Despair.ogg",
	L"01 - Through The Horned Gate.ogg",
	L"02 - Night Marauders.ogg",
	L"03 - The Goatriders Horde.ogg",
	L"04 - Trial Of Champions.ogg",
	L"05 - God Of The Cold White Silence.ogg",
	L"06 - Forest King.ogg",
	L"07 - Demon's Blade.ogg",
	L"08 - The Great Hall Of Feasting.ogg",
	L"09 - Infinite Legions.ogg",
	L"10 - Assassins Of The Light.ogg",
	L"11 - Black Spire.ogg",
	L"12 - The Hydra's Teeth.ogg",
	L"13 - Rejoice In The Fire Of Man's Demise.ogg",
	L"1.01 - Dragonborn.ogg",
	L"1.02 - Awake.ogg",
	L"1.03 - From Past to Present.ogg",
	L"1.04 - Unbroken Road.ogg",
	L"1.05 - Ancient Stones.ogg",
	L"1.06 - The City Gates.ogg",
	L"1.07 - Silent Footsteps.ogg",
	L"1.08 - Dragonsreach.ogg",
	L"1.09 - Tooth and Claw.ogg",
	L"1.10 - Under an Ancient Sun.ogg",
	L"1.11 - Death or Sovngarde.ogg",
	L"1.12 - Masser.ogg",
	L"1.13 - Distant Horizons.ogg",
	L"1.14 - Dawn.ogg",
	L"1.15 - The Jerall Mountains.ogg",
	L"1.16 - Steel on Steel.ogg",
	L"1.17 - Secunda.ogg",
	L"1.18 - Imperial Throne.ogg",
	L"2.01 - Frostfall.ogg",
	L"2.02 - Night Without Stars.ogg",
	L"2.03 - Into Darkness.ogg",
	L"2.04 - Kyne's Peace.ogg",
	L"2.05 - Unbound.ogg",
	L"2.06 - Far Horizons.ogg",
	L"2.07 - A Winter's Tale.ogg",
	L"2.08 - The Bannered Mare.ogg",
	L"2.09 - The Streets Of Whiterun.ogg",
	L"2.10 - One They Fear.ogg",
	L"2.11 - The White River.ogg",
	L"2.12 - Silence Unbroken.ogg",
	L"2.13 - Standing Stones.ogg",
	L"2.14 - Beneath The Ice.ogg",
	L"2.15 - Tundra.ogg",
	L"2.16 - Journey's End.ogg",
	L"3.01 - Before the Storm.ogg",
	L"3.02 - A Chance Meeting.ogg",
	L"3.03 - Out of the Cold.ogg",
	L"3.04 - Around the Fire.ogg",
	L"3.05 - Shadows and Echoes.ogg",
	L"3.06 - Caught off Guard.ogg",
	L"3.07 - Aurora.ogg",
	L"3.08 - Blood and Steel.ogg",
	L"3.09 - Towers and Shadows.ogg",
	L"3.10 - Seven Thousand Steps.ogg",
	L"3.11 - Solitude.ogg",
	L"3.12 - Watch the Skies.ogg",
	L"3.13 - The Gathering Storm.ogg",
	L"3.14 - Sky Above, Voice Within.ogg",
	L"3.15 - Death in the Darkness.ogg",
	L"3.16 - Shattered Shields.ogg",
	L"3.17 - Sovngarde.ogg",
	L"3.18 - Wind Guide You.ogg",
	L"4.01 - Skyrim Atmospheres.ogg",
	L"Sonic the Hedgehog 2 - 01 - Opening Theme.flac",
	L"Sonic the Hedgehog 2 - 02 - Options.flac",
	L"Sonic the Hedgehog 2 - 03 - Emerald Hill Zone.flac",
	L"Sonic the Hedgehog 2 - 04 - Act Clear.flac",
	L"Sonic the Hedgehog 2 - 05 - Chemical Plant Zone.flac",
	L"Sonic the Hedgehog 2 - 06 - Drowning.flac",
	L"Sonic the Hedgehog 2 - 07 - Aquatic Ruin Zone.flac",
	L"Sonic the Hedgehog 2 - 08 - Casino Night Zone.flac",
	L"Sonic the Hedgehog 2 - 09 - Hill Top Zone.flac",
	L"Sonic the Hedgehog 2 - 10 - Mystic Cave Zone.flac",
	L"Sonic the Hedgehog 2 - 11 - Oil Ocean Zone.flac",
	L"Sonic the Hedgehog 2 - 12 - Metropolis Zone.flac",
	L"Sonic the Hedgehog 2 - 13 - Sky Chase Zone.flac",
	L"Sonic the Hedgehog 2 - 14 - Wing Fortress Zone.flac",
	L"Sonic the Hedgehog 2 - 15 - Death Egg Zone.flac",
	L"Sonic the Hedgehog 2 - 16 - Dr_ Robotnik's Theme.flac",
	L"Sonic the Hedgehog 2 - 17 - Final Boss.flac",
	L"Sonic the Hedgehog 2 - 18 - Continue.flac",
	L"Sonic the Hedgehog 2 - 19 - Ending Theme.flac",
	L"Sonic the Hedgehog 2 - 20 - Staff Roll.flac",
	L"Sonic the Hedgehog 2 - 21 - Game Over.flac",
	L"Sonic the Hedgehog 2 - 22 - Invincibility.flac",
	L"Sonic the Hedgehog 2 - 23 - Super Sonic.flac",
	L"Sonic the Hedgehog 2 - 24 - Extra Life.flac",
	L"Sonic the Hedgehog 2 - 25 - 2 Player Results.flac",
	L"Sonic the Hedgehog 2 - 26 - Emerald Hill (2 Player).flac",
	L"Sonic the Hedgehog 2 - 27 - Casino Night (2 Player).flac",
	L"Sonic the Hedgehog 2 - 28 - Mystic Cave (2 Player).flac",
	L"Sonic the Hedgehog 2 - 29 - Special Stage.flac",
	L"Sonic the Hedgehog 2 - 30 - Chaos Emerald.flac",
	L"Sonic the Hedgehog 2 - 31 - Unused (Sound Test 10).flac",
	L"Sonic the Hedgehog 2 - 01 - Opening Theme.ogg",
	L"Sonic the Hedgehog 2 - 02 - Options.ogg",
	L"Sonic the Hedgehog 2 - 03 - Emerald Hill Zone.ogg",
	L"Sonic the Hedgehog 2 - 04 - Act Clear.ogg",
	L"Sonic the Hedgehog 2 - 05 - Chemical Plant Zone.ogg",
	L"Sonic the Hedgehog 2 - 06 - Drowning.ogg",
	L"Sonic the Hedgehog 2 - 07 - Aquatic Ruin Zone.ogg",
	L"Sonic the Hedgehog 2 - 08 - Casino Night Zone.ogg",
	L"Sonic the Hedgehog 2 - 09 - Hill Top Zone.ogg",
	L"Sonic the Hedgehog 2 - 10 - Mystic Cave Zone.ogg",
	L"Sonic the Hedgehog 2 - 11 - Oil Ocean Zone.ogg",
	L"Sonic the Hedgehog 2 - 12 - Metropolis Zone.ogg",
	L"Sonic the Hedgehog 2 - 13 - Sky Chase Zone.ogg",
	L"Sonic the Hedgehog 2 - 14 - Wing Fortress Zone.ogg",
	L"Sonic the Hedgehog 2 - 15 - Death Egg Zone.ogg",
	L"Sonic the Hedgehog 2 - 16 - Dr_ Robotnik's Theme.ogg",
	L"Sonic the Hedgehog 2 - 17 - Final Boss.ogg",
	L"Sonic the Hedgehog 2 - 18 - Continue.ogg",
	L"Sonic the Hedgehog 2 - 19 - Ending Theme.ogg",
	L"Sonic the Hedgehog 2 - 20 - Staff Roll.ogg",
	L"Sonic the Hedgehog 2 - 21 - Game Over.ogg",
	L"Sonic the Hedgehog 2 - 22 - Invincibility.ogg",
	L"Sonic the Hedgehog 2 - 23 - Super Sonic.ogg",
	L"Sonic the Hedgehog 2 - 24 - Extra Life.ogg",
	L"Sonic the Hedgehog 2 - 25 - 2 Player Results.ogg",
	L"Sonic the Hedgehog 2 - 26 - Emerald Hill (2 Player).ogg",
	L"Sonic the Hedgehog 2 - 27 - Casino Night (2 Player).ogg",
	L"Sonic the Hedgehog 2 - 28 - Mystic Cave (2 Player).ogg",
	L"Sonic the Hedgehog 2 - 29 - Special Stage.ogg",
	L"Sonic the Hedgehog 2 - 30 - Chaos Emerald.ogg",
	L"Sonic the Hedgehog 2 - 31 - Unused (Sound Test 10).ogg",
};

unsigned MainScreen::handle_event(const SDL_Event &event){
	if (!!this->listview)
		return this->listview->handle_event(event);
	return GUIElement::handle_event(event);
}

void MainScreen::update(){
	if (!!this->listview){
		this->listview->update();
		return;
	}
	std::wstringstream stream;
	parse_into_hms(stream, player.get_current_time());
	stream <<" / ";
	parse_into_hms(stream, this->sui->get_current_total_time());
	stream <<std::endl
		<<this->sui->get_metadata();
	this->sui->draw_picture();
	this->sui->get_font()->draw_text(stream.str(), 0, 0, this->sui->get_bounding_square(), 2);
	GUIElement::update();
}

void MainScreen::gui_signal(const GuiSignal &signal){
	if (signal.type != SignalType::BUTTON_SIGNAL)
		return;
	switch ((ButtonSignal)signal.data.button_signal){
		case ButtonSignal::PLAY:
			this->player.request_hardplay();
			break;
		case ButtonSignal::PAUSE:
			this->player.request_pause();
			break;
		case ButtonSignal::STOP:
			this->player.request_stop();
			break;
		case ButtonSignal::LOAD:
			/*
			if (!this->listview){
				std::vector<std::wstring> files;
				for (auto p : test_list)
					files.push_back(p);
				this->listview.reset(new ListView(this->sui, this, files, 0));
				this->children.push_back(this->listview);
			}
			*/
			{
				GuiSignal signal;
				signal.type = SignalType::MAINSCREEN_LOAD;
				this->parent->gui_signal(signal);
			}
			break;
		case ButtonSignal::PREVIOUS:
			this->player.request_previous();
			break;
		case ButtonSignal::SEEKBACK:
			this->player.request_seek(-5);
			break;
		case ButtonSignal::SEEKFORTH:
			this->player.request_seek(5);
			break;
		case ButtonSignal::NEXT:
			this->player.request_next();
			break;
	}
}

void MainScreen::prepare_buttons(){
	static const char *button_paths[] = {
		BASE_PATH "button_play.png",
		BASE_PATH "button_pause.png",
		BASE_PATH "button_stop.png",
		BASE_PATH "button_load.png",
		BASE_PATH "button_previous.png",
		BASE_PATH "button_seekback.png",
		BASE_PATH "button_seekforth.png",
		BASE_PATH "button_next.png",
	};
	const size_t n = sizeof(button_paths) / sizeof(*button_paths);
	surface_t button_surfaces[n];
	int i = 0;
	const int square = this->sui->get_bounding_square();
	for (auto p : button_paths){
		auto image = load_image_from_file(p);
		if (!image)
			throw UIInitializationException("Failed to load a button.");
		image = bind_surface_to_square(image, square / 4);
		if (!image)
			throw UIInitializationException("Failed to transform a button.");
		button_surfaces[i++] = image;
	}
	int w = button_surfaces[0]->w;
	int h = button_surfaces[0]->h;
	auto new_surface = create_rgba_surface(w * 4, h * 2);
	i = 0;
	SDL_Rect rects[n];
	for (auto &surface : button_surfaces){
		SDL_Rect dst = { w * (i % 4), h * (i / 4), w, h, };
		rects[i] = dst;
		SDL_BlitSurface(button_surfaces[i].get(), 0, new_surface.get(), &dst);
		i++;
	}
	this->tex_buttons.set_renderer(this->sui->get_renderer());
	this->tex_buttons.load(new_surface);
	for (i = 0; i < n; i++){
		Subtexture st(this->tex_buttons, rects[i]);
		boost::shared_ptr<GraphicButton> button(new GraphicButton(this->sui, this));
		button->set_graphic(st);
		button->set_position(rects[i].x, rects[i].y + square);
		button->set_signal_value(i);
		this->children.push_back(button);
	}
}
