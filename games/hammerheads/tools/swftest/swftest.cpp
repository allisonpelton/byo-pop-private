// hammerheads_swftest: the shared SWF player (engines/swf) on the how-to-play movie without the game, headless.
//
//     hammerheads_swftest [-gamedir=<dir>] [-dump <frame>] [-render <out prefix> <frame>[,<frame>...]]
//
// Plays images\howtoplay.swf from <dir> (default assets/hammerheads) through the way the game's widget does, a tick at a time
// until its root timeline has shown its last frame (or twice its frame count of ticks have gone by), and prints the movie's
// size, rate and frame count and what the file holds that the player skips (SwfMovie::GetUnsupported). -dump prints the display
// list at a root frame; -render saves the root frames named as <out prefix>_<frame>.png. The exit code is 1 when the movie does
// not load.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include <SDL.h>  // main becomes SDL_main, which the platform's SDL2main calls
#include <stb_image_write.h>

#include "SwfMovie.h"
#include "platform/VFS.h"

using namespace Sexy;

namespace
{
const char* const MOVIE_FILE = "images\\howtoplay.swf";

class Listener : public SwfListener
{
public:
	void SwfCommand(const std::string& theCommand, const std::string& theArgs) override
	{
		printf("FSCommand %s(%s)\n", theCommand.c_str(), theArgs.c_str());
	}
};

void SavePicture(SwfMovie& theMovie, const std::string& thePath)
{
	int aWidth = theMovie.GetWidth(), aHeight = theMovie.GetHeight();
	std::vector<uint32_t> aBits((size_t)aWidth * aHeight, 0);
	theMovie.Invalidate();
	theMovie.Render(aBits.data(), aWidth, aHeight, aWidth);
	std::vector<unsigned char> aRgba(aBits.size() * 4);
	for (size_t i = 0; i < aBits.size(); i++)
	{
		aRgba[i * 4 + 0] = (aBits[i] >> 16) & 0xff;
		aRgba[i * 4 + 1] = (aBits[i] >> 8) & 0xff;
		aRgba[i * 4 + 2] = aBits[i] & 0xff;
		aRgba[i * 4 + 3] = (aBits[i] >> 24) & 0xff;
	}
	stbi_write_png(thePath.c_str(), aWidth, aHeight, 4, aRgba.data(), aWidth * 4);
}
}

int main(int argc, char** argv)
{
	std::string aGameDir = "assets/hammerheads";
	int aDumpFrame = -1;
	std::string aPrefix;
	std::set<int> aRenderFrames;
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(argv[i], "-gamedir=", 9) == 0)
			aGameDir = argv[i] + 9;
		else if (strcmp(argv[i], "-dump") == 0 && i + 1 < argc)
			aDumpFrame = atoi(argv[++i]);
		else if (strcmp(argv[i], "-render") == 0 && i + 2 < argc)
		{
			aPrefix = argv[++i];
			for (char* aPart = strtok(argv[++i], ","); aPart != nullptr; aPart = strtok(nullptr, ","))
				aRenderFrames.insert(atoi(aPart));
		}
	}

	Platform::VFS::Get().SetBaseDir(aGameDir);
	Listener aListener;
	SwfMovie aMovie;
	aMovie.mListener = &aListener;
	if (!aMovie.Load(MOVIE_FILE))
	{
		printf("%s: %s\n", MOVIE_FILE, aMovie.GetError().c_str());
		return 1;
	}
	printf("%s: %dx%d, %.1f frames a second, %d frames\n", MOVIE_FILE, aMovie.GetWidth(), aMovie.GetHeight(), aMovie.GetFrameRate(),
		aMovie.GetFrameCount());

	aMovie.Play();
	int aTicks = 0;
	int aLast = aMovie.GetFrameCount() * 2;
	int aFrame = -1;
	while (aMovie.IsPlaying() && aTicks < aLast)
	{
		aMovie.NextFrame();
		aTicks++;
		int aNow = aMovie.GetCurrentFrame();
		if (aNow == aFrame)
			continue;
		aFrame = aNow;
		if (aRenderFrames.count(aNow) != 0)
			SavePicture(aMovie, aPrefix + "_" + std::to_string(aNow) + ".png");
		if (aNow == aDumpFrame)
			printf("%s", aMovie.DumpDisplayList().c_str());
	}
	printf("%d ticks, stopped at frame %d, playing %d\n", aTicks, aMovie.GetCurrentFrame(), aMovie.IsPlaying() ? 1 : 0);
	for (const auto& anItem : aMovie.GetUnsupported())
		printf("unsupported: %s x%d\n", anItem.first.c_str(), anItem.second);
	return 0;
}
