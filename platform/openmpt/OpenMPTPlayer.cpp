#include "OpenMPTPlayer.h"
#include "platform/Audio.h"

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_ext.h>
#include <SDL.h>
#include <cmath>
#include <cstring>
#include <vector>

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
//
// The tempo and the channels' volumes (BASS's BPM and VOL_CHAN music attributes) go through libopenmpt's
// interactive interfaces. BASS keeps both when the music is played from another position; libopenmpt works out a
// new position by playing the module up to it, which sets them back, so they are set again after each seek.
class OpenMPTModule : public ModulePlayer
{
public:
	OpenMPTModule(openmpt_module_ext* theModuleExt, int theRate) :
		mModuleExt(theModuleExt), mModule(openmpt_module_ext_get_module(theModuleExt)), mRate(theRate)
	{
		openmpt_module_ext_get_interface(mModuleExt, LIBOPENMPT_EXT_C_INTERFACE_INTERACTIVE, &mInteractive,
			sizeof(mInteractive));
		mHasInteractive3 = openmpt_module_ext_get_interface(mModuleExt, LIBOPENMPT_EXT_C_INTERFACE_INTERACTIVE3,
			&mInteractive3, sizeof(mInteractive3)) != 0;
		openmpt_module_set_render_param(mModule, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, 2);
		openmpt_module_set_render_param(mModule, OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT, 100);
		openmpt_module_set_repeat_count(mModule, -1);
		mLoop = true;
	}

	~OpenMPTModule() override
	{
		openmpt_module_ext_destroy(mModuleExt);
	}

	void SetPosition(int theOrder, int theRow) override
	{
		openmpt_module_set_position_order_row(mModule, theOrder, theRow);
		if (mTempo > 0)
			ApplyTempo();
		for (int i = 0; i < (int)mChannelVolumes.size(); i++)
			if (mChannelVolumes[i] >= 0)
				mInteractive.set_channel_volume(mModuleExt, i, mChannelVolumes[i] / 64.0);
	}

	int GetTempo() override
	{
		return (int)std::lround(openmpt_module_get_current_tempo2(mModule));
	}

	void SetTempo(int theBPM) override
	{
		mTempo = theBPM;
		ApplyTempo();
	}

	void SetChannelVolume(int theChannel, int theVolume) override
	{
		if (theChannel < 0 || theChannel >= openmpt_module_get_num_channels(mModule))
			return;
		theVolume = theVolume < 0 ? 0 : (theVolume > 64 ? 64 : theVolume);
		if ((int)mChannelVolumes.size() <= theChannel)
			mChannelVolumes.resize(theChannel + 1, -1);
		mChannelVolumes[theChannel] = theVolume;
		mInteractive.set_channel_volume(mModuleExt, theChannel, theVolume / 64.0);
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
	void ApplyTempo()
	{
		if (mHasInteractive3)
			mInteractive3.set_current_tempo2(mModuleExt, (double)mTempo);
	}

	openmpt_module_ext* mModuleExt;
	openmpt_module* mModule;
	int mRate;
	bool mLoop;
	openmpt_module_ext_interface_interactive mInteractive = {};
	openmpt_module_ext_interface_interactive3 mInteractive3 = {};
	bool mHasInteractive3 = false;
	int mTempo = 0;                     // the BPM last set, 0 for none
	std::vector<int> mChannelVolumes;   // 0 to 64 each, -1 for a channel never set
};

bool IsMO3(const std::vector<uint8_t>& theFile)
{
	return theFile.size() > 4 && !memcmp(theFile.data(), "MO3", 3);
}

void LogError(int theError, const char* theMessage)
{
	SDL_Log("libopenmpt: error %d: %s", theError, theMessage ? theMessage : "");
}

std::unique_ptr<ModulePlayer> CreateModule(const std::vector<uint8_t>& theFile, int theRate)
{
	int anError = 0;
	const char* aMessage = nullptr;
	openmpt_module_ext* aModule = openmpt_module_ext_create_from_memory(theFile.data(), theFile.size(),
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

void RegisterOpenMPTModules()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Mixer::RegisterModuleFormat(IsMO3, CreateModule);
}

}
