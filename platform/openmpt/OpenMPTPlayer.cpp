#include "OpenMPTPlayer.h"
#include "platform/Audio.h"

#include <libopenmpt/libopenmpt.h>
#include <SDL.h>
#include <cstring>

namespace Platform
{

namespace
{

// One loaded MO3 module, rendered by libopenmpt at the mixer rate inside the audio callback.
//
// BASS 2.0 (the originals' BassMusicInterface) plays MO3 with linear interpolation and no volume ramping
// unless asked, so the renderer uses a 2-tap (linear) filter rather than libopenmpt's 8-tap default.
// PlayMusic(id, MAKELONG(order, row)) maps to openmpt_module_set_position_order_row; looping songs
// repeat forever (BASS_MUSIC_LOOP), the others stop at the end of the module.
class OpenMPTModule : public ModulePlayer
{
public:
	OpenMPTModule(openmpt_module* theModule, int theRate) : mModule(theModule), mRate(theRate)
	{
		openmpt_module_set_render_param(mModule, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, 2);
		openmpt_module_set_render_param(mModule, OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT, 100);
		openmpt_module_set_repeat_count(mModule, -1);
		mLoop = true;
	}

	~OpenMPTModule() override
	{
		openmpt_module_destroy(mModule);
	}

	void SetPosition(int theOrder, int theRow) override
	{
		openmpt_module_set_position_order_row(mModule, theOrder, theRow);
	}

	bool GetPosition(int& theOrder, int& theRow) override
	{
		theOrder = openmpt_module_get_current_order(mModule);
		theRow = openmpt_module_get_current_row(mModule);
		return true;
	}

	bool Render(float* theOut, int theFrames, bool theLoop) override
	{
		if (theLoop != mLoop)
		{
			mLoop = theLoop;
			openmpt_module_set_repeat_count(mModule, theLoop ? -1 : 0);
		}
		int done = 0;
		while (done < theFrames)
		{
			size_t got = openmpt_module_read_interleaved_float_stereo(mModule, mRate, (size_t)(theFrames - done),
				theOut + done * 2);
			if (got == 0)
				break;
			done += (int)got;
		}
		if (done < theFrames)
		{
			memset(theOut + done * 2, 0, sizeof(float) * 2 * (size_t)(theFrames - done));
			return false;
		}
		return true;
	}

private:
	openmpt_module* mModule;
	int mRate;
	bool mLoop;
};

bool IsMO3(const std::vector<uint8_t>& theFile)
{
	return theFile.size() > 4 && !memcmp(theFile.data(), "MO3", 3);
}

// An OXM: an XM whose samples are Ogg Vorbis streams (FMOD 3's "Ogg XM", Bonnie's Bookstore), which libxmp-lite cannot
// read: the XM header and an Ogg page among the samples
bool IsOXM(const std::vector<uint8_t>& theFile)
{
	static const char kHeader[] = "Extended Module: ";
	if (theFile.size() < 0x150 || memcmp(theFile.data(), kHeader, sizeof(kHeader) - 1) != 0)
		return false;
	for (size_t i = 0x150; i + 4 <= theFile.size(); i++)
		if (!memcmp(theFile.data() + i, "OggS", 4))
			return true;
	return false;
}

void LogError(int theError, const char* theMessage)
{
	SDL_Log("libopenmpt: error %d: %s", theError, theMessage ? theMessage : "");
}

std::unique_ptr<ModulePlayer> CreateModule(const std::vector<uint8_t>& theFile, int theRate)
{
	int anError = 0;
	const char* aMessage = nullptr;
	openmpt_module* aModule = openmpt_module_create_from_memory2(theFile.data(), theFile.size(),
		openmpt_log_func_silent, nullptr, openmpt_error_func_default, nullptr, &anError, &aMessage, nullptr);
	if (aModule == nullptr)
	{
		LogError(anError, aMessage);
		openmpt_free_string(aMessage);
		return nullptr;
	}
	openmpt_free_string(aMessage);
	return std::unique_ptr<ModulePlayer>(new OpenMPTModule(aModule, theRate));
}

}

std::unique_ptr<ModulePlayer> CreateOpenMPTModule(const std::vector<uint8_t>& theFile, int theRate)
{
	return CreateModule(theFile, theRate);
}

void RegisterOpenMPTModules()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Mixer::RegisterModuleFormat(IsMO3, CreateModule);
}

void RegisterOpenMPTOggXM()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Mixer::RegisterModuleFormat(IsOXM, CreateModule);
}

}
