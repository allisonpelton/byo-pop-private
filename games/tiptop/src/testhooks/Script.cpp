// port: test hooks: TipTop's -script commands besides the framework's
//
//   canvasshot <file>           the whole screen of a -canvas run, after the next frame (Canvas.cpp)
//   musicwav <file> <seconds>   the music from its start, rendered into a WAV file without the audio device
//                               (MusicSynth.cpp; with -soundfont=<file>, by another bank)
// and the standard test commands (state, goto, level, set, win, lose, seed, move; Commands.cpp)

#include "TipTopApp.h"
#include "MusicSynth.h"

#include <SDL.h>

#include <cstdlib>

using namespace Sexy;

bool TipTopApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "canvasshot")
	{
		StartCanvasShot(theCommand.mArgs);
		return true;
	}
	if (theCommand.mCommand == "musicwav")
	{
		size_t aSpace = theCommand.mArgs.find_last_of(' ');
		int aSeconds = (aSpace != std::string::npos) ? atoi(theCommand.mArgs.c_str() + aSpace + 1) : 0;
		if (aSeconds <= 0 || aSeconds > 600)
			SDL_Log("musicwav %s: expected <file> <seconds>, up to 600", theCommand.mArgs.c_str());
		else
			tiptop::RenderMusicWav(theCommand.mArgs.substr(0, aSpace), aSeconds);
		return true;
	}
	return HandleTestCommand(theCommand);
}
