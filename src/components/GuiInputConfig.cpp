#include "GuiInputConfig.h"
#include "GuiGameList.h"
#include <iostream>
#include <fstream>
#include "../Log.h"

extern bool DEBUG; //defined in main.cpp

std::string GuiInputConfig::sInputs[] = { "UNKNOWN", "UP", "DOWN", "LEFT", "RIGHT", "BUTTON1 (Accept)", "BUTTON2 (Back)", "START (Menu)", "SELECT (Jump-to-letter)", "PAGE UP", "PAGE DOWN" }; //must be same order as InputManager::InputButton enum; only add to the end to preserve backwards compatibility
int GuiInputConfig::sInputCount = 11;

GuiInputConfig::GuiInputConfig()
{
	mInputNum = 1;
	mDone = false;
	mLastAxis = -1;

	Renderer::registerComponent(this);
	InputManager::registerComponent(this);

	if(SDL_NumJoysticks() < 1)
	{
		std::cerr << "Error - GuiInputConfig found no SDL joysticks!\n";
		mJoystick = NULL;
		mDone = true;
		return;
	}else{
		LOG(LogInfo) << "Opening joystick \"" << SDL_JoystickName(0) << "\" for configuration...";
		mJoystick = SDL_JoystickOpen(0);
	}
}

GuiInputConfig::~GuiInputConfig()
{
	Renderer::unregisterComponent(this);
	InputManager::unregisterComponent(this);
}

void GuiInputConfig::onRender()
{
	Renderer::drawRect(0, 0, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0xFFFFFFFF);

	Font* font = Renderer::getDefaultFont(Renderer::MEDIUM);

	int height = font->getHeight() + 6;


	Renderer::drawCenteredText("It looks like you have a joystick plugged in!", 0, 2, 0x000000FF, font);
	Renderer::drawCenteredText("POV hats (some D-Pads) are automatically mapped to directions.", 0, height, 0x000000FF, font);
	Renderer::drawCenteredText("You can press a keyboard key to skip any input.", 0, height * 2, 0x000000FF, font);
	Renderer::drawCenteredText("If you want to remap later, delete ~/.emulationstation/es_input.cfg.", 0, height * 3, 0x000000FF, font);
	Renderer::drawCenteredText("This interface only configures the first joystick plugged in.", 0, height * 4, 0x000000FF, font);
	Renderer::drawCenteredText("Remember - you'll need to set up your emulator separately!", 0, Renderer::getScreenHeight() - height, 0x000000FF, font);

	if(mDone)
		Renderer::drawCenteredText("All done! Press a key or button to save.", 0, height * 5, 0x00BB00FF, font);
	else
		Renderer::drawCenteredText("Please press the axis/button for " + sInputs[mInputNum], 0, height * 5, 0x00C000FF, font);
}

void GuiInputConfig::onInput(InputManager::InputButton button, bool keyDown)
{
	if(mDone)
	{
		if(InputManager::lastEvent->type == SDL_KEYUP || InputManager::lastEvent->type == SDL_JOYBUTTONDOWN)
		{
			LOG(LogDebug) << "	finishing configuration...";

			writeConfig();

			if(mJoystick)
				SDL_JoystickClose(mJoystick);

			LOG(LogDebug) << "Config complete. Closed joystick, loading input then opening GuiGameList.";

			InputManager::loadConfig();
			delete this;
			GuiGameList::create();
		}
		return;
	}

	SDL_Event* event = InputManager::lastEvent;
	if(event->type == SDL_KEYUP)
	{
		LOG(LogDebug) << "	[KEYUP] (skipping)";

		//keyboard key pressed; skip and continue
		mInputNum++;
	}

	if(event->type == SDL_JOYBUTTONDOWN)
	{
		LOG(LogDebug) << "	[JOYBUTTONDOWN] button " << event->jbutton.button;

		mButtonMap[event->jbutton.button] = (InputManager::InputButton)mInputNum;
		LOG(LogInfo) << "	Mapping " << sInputs[mInputNum] << " to button " << (int)event->jbutton.button;
		mInputNum++;
	}

	if(event->type == SDL_JOYAXISMOTION)
	{
		LOG(LogDebug) << "	[JOYAXISMOTION] axis " << event->jaxis.axis << ", value " << event->jaxis.value;

		if(event->jaxis.axis == mLastAxis)
		{
			if(event->jaxis.value < InputManager::deadzone && event->jaxis.value > -InputManager::deadzone)
				mLastAxis = -1;
			return;
		}
		if(event->jaxis.value > InputManager::deadzone)
		{
			mAxisPosMap[event->jaxis.axis] = (InputManager::InputButton)mInputNum;
			mInputNum++;
			mLastAxis = event->jaxis.axis;
			LOG(LogInfo) << "	Mapping " << sInputs[mInputNum - 1] << " to axis+ " << mLastAxis;
		}else if(event->jaxis.value < -InputManager::deadzone)
		{
			mAxisNegMap[event->jaxis.axis] = (InputManager::InputButton)mInputNum;
			mInputNum++;
			mLastAxis = event->jaxis.axis;
			LOG(LogInfo) << "	Mapping " << sInputs[mInputNum - 1] << " to axis- " << mLastAxis;
		}
	}

	if(mInputNum >= sInputCount)
	{
		mDone = true;
		return;
	}
}

void GuiInputConfig::writeConfig()
{
	LOG(LogDebug) << "	Writing config...";

	std::string path = InputManager::getConfigPath();

	std::ofstream file(path.c_str());

	if(SDL_JoystickName(0))
		file << "JOYNAME " << SDL_JoystickName(0) << "\n";

	typedef std::map<int, InputManager::InputButton>::iterator it_type;
	for(it_type iter = mButtonMap.begin(); iter != mButtonMap.end(); iter++)
	{
		file << "BUTTON " << iter->first << " " << iter->second << "\n";
	}

	for(it_type iter = mAxisPosMap.begin(); iter != mAxisPosMap.end(); iter++)
	{
		file << "AXISPOS " << iter->first << " " << iter->second << "\n";
	}

	for(it_type iter = mAxisNegMap.begin(); iter != mAxisNegMap.end(); iter++)
	{
		file << "AXISNEG " << iter->first << " " << iter->second << "\n";
	}

	file.close();

	LOG(LogDebug) << "	Write complete.\n";
}
