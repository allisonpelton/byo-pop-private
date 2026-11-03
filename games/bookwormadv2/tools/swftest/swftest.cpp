// bwa2_swftest: SwfMovie (swf/) without the game, headless: no window, no sound.
//
//     bwa2_swftest [-gamedir=<dir>] [-size=<w>x<h>] [-noplay] [<file>...]
//     bwa2_swftest [-gamedir=<dir>] [-size=<w>x<h>] -render <file> <frame>[,<frame>...] <out prefix>
//     bwa2_swftest [-gamedir=<dir>] -dump <file> <frame>
//     bwa2_swftest [-gamedir=<dir>] -chars <file> [<id>]
//     bwa2_swftest [-gamedir=<dir>] [-size=<w>x<h>] -verify <file>
//     bwa2_swftest [-gamedir=<dir>] [-size=<w>x<h>] -bench <file> <frame> [<count>]
//     bwa2_swftest [-gamedir=<dir>] -selftest [<file>]
//
// With no file it plays every .swf under <dir>/images/cutscenes (default assets/bookwormadv2) through, as the
// game's cutscene widget does: a tick at a time, rendering each frame, answering the movie's "ready" FSCommand
// with Play, until the root timeline has shown its last frame (or the tick count passes twice its frame count).
// It prints one line for each file: the ticks played, the FSCommands heard with the root frame of each, the
// average and the longest time to render a frame, and what the file holds that the player skips. The exit code
// is the number of files that failed to load or to reach their last frame. -noplay leaves the movie's "ready" unanswered,
// to see what a movie does when the host does not tell it to play.
//
// -render plays <file> (a path under <dir>, or a bare name in images/cutscenes) the same way, rendering every
// frame (so that the sprites are in the state a viewer would see) but saving only the root frames named, each as
// <out prefix>_<frame>.png. -size renders at a size other than the movie's own. -dump plays to a root frame and
// prints the display list there (SwfMovie::DumpDisplayList). -chars lists the file's characters, or one in full.
// -verify plays <file> twice at once, one SwfMovie that draws only what changed from frame to frame, as in the game,
// and one that draws every frame whole, and compares the two pictures at every frame. -bench plays to a root frame and
// draws it whole <count> times (default 20), to time a drawing of all of a frame. -selftest checks the playback
// calls (Play, Stop, GotoFrame, looping, the FSCommands) on a movie (default intro.swf).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL.h>  // main becomes SDL_main, which the platform's SDL2main calls
#include <stb_image_write.h>

#include "SwfMovie.h"
#include "SwfTypes.h"
#include "platform/VFS.h"

using namespace Sexy;

namespace {

std::string gGameDir = "assets/bookwormadv2";
int gWidth = 0, gHeight = 0;
int gDumpFrame = -1;  // the root frame whose display list to print
bool gAnswerReady = true;  // whether to Play when the movie says "ready" (-noplay: it does not)

class TestListener : public SwfListener {
public:
	SwfMovie* mMovie = nullptr;
	std::string mHeard;  // "ready@0 done@939"

	void SwfCommand(const std::string& theCommand, const std::string& theArgs) override
	{
		char aBuf[64];
		snprintf(aBuf, sizeof(aBuf), "%s%s@%d", mHeard.empty() ? "" : " ", theCommand.c_str(), mMovie->GetCurrentFrame());
		mHeard += aBuf;
		if (!theArgs.empty())
			mHeard += "(" + theArgs + ")";
		// What the cutscene widget does: the movie is ready, so play it
		if (theCommand == "ready" && gAnswerReady)
			mMovie->Play();
	}
};

// The files under images/cutscenes, as the game names them, sorted
std::vector<std::string> FindMovies()
{
	std::vector<std::string> aFiles;
	std::filesystem::path aRoot = std::filesystem::path(gGameDir) / "images" / "cutscenes";
	std::error_code ec;
	for (std::filesystem::directory_iterator anItr(aRoot, ec), anEnd; !ec && anItr != anEnd; anItr.increment(ec)) {
		std::string aName = anItr->path().filename().string();
		if (aName.size() > 4 && aName.substr(aName.size() - 4) == ".swf")
			aFiles.push_back("images\\cutscenes\\" + aName);
	}
	std::sort(aFiles.begin(), aFiles.end());
	return aFiles;
}

bool WritePng(const std::string& thePath, const std::vector<uint32_t>& thePixels, int theWidth, int theHeight)
{
	std::vector<unsigned char> aRGBA((size_t)theWidth * theHeight * 4);
	for (size_t i = 0; i < (size_t)theWidth * theHeight; i++) {
		uint32_t c = thePixels[i];
		aRGBA[i * 4] = (c >> 16) & 0xff;
		aRGBA[i * 4 + 1] = (c >> 8) & 0xff;
		aRGBA[i * 4 + 2] = c & 0xff;
		aRGBA[i * 4 + 3] = (c >> 24) & 0xff;
	}
	return stbi_write_png(thePath.c_str(), theWidth, theHeight, 4, &aRGBA[0], theWidth * 4) != 0;
}

// Plays one movie. When theFrames is not null, the root frames in it are saved as PNGs named by thePrefix.
// Returns whether the root timeline reached its last frame.
bool Play(const std::string& thePath, const std::vector<int>* theFrames, const std::string& thePrefix)
{
	SwfMovie aMovie;
	TestListener aListener;
	aListener.mMovie = &aMovie;
	aMovie.mListener = &aListener;
	if (!aMovie.Load(thePath)) {
		printf("%s: FAIL %s\n", thePath.c_str(), aMovie.GetError().c_str());
		return false;
	}
	int w = gWidth > 0 ? gWidth : aMovie.GetWidth();
	int h = gHeight > 0 ? gHeight : aMovie.GetHeight();
	std::vector<uint32_t> aPixels((size_t)w * h);
	int aFrames = aMovie.GetFrameCount();
	int aLastWanted = 0;
	if (theFrames != nullptr) {
		for (size_t i = 0; i < theFrames->size(); i++)
			aLastWanted = std::max(aLastWanted, (*theFrames)[i]);
	}

	std::vector<int> aSaved;
	int aTicks = 0;
	double aTotal = 0, aMax = 0;
	int aSlowestFrame = 0;
	std::vector<double> aTimes;
	bool aReachedEnd = false;
	for (int aTick = 0; aTick < aFrames * 2; aTick++) {
		aMovie.NextFrame();
		aTicks++;
		auto aStart = std::chrono::steady_clock::now();
		if (gDumpFrame < 0)
			aMovie.Render(aPixels.data(), w, h, w);
		double aMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - aStart).count();
		aTotal += aMs;
		if (aMs > aMax) {
			aMax = aMs;
			aSlowestFrame = aMovie.GetCurrentFrame();
		}
		aTimes.push_back(aMs);

		int aFrame = aMovie.GetCurrentFrame();
		if (theFrames != nullptr && std::find(theFrames->begin(), theFrames->end(), aFrame) != theFrames->end() &&
		    std::find(aSaved.begin(), aSaved.end(), aFrame) == aSaved.end()) {
			aSaved.push_back(aFrame);
			char aName[32];
			snprintf(aName, sizeof(aName), "_%d.png", aFrame);
			std::string aFile = thePrefix + aName;
			if (!WritePng(aFile, aPixels, w, h))
				printf("FAIL cannot write %s\n", aFile.c_str());
		}
		if (aFrame == gDumpFrame) {
			printf("%s", aMovie.DumpDisplayList().c_str());
			return true;
		}
		if (aFrame == aFrames - 1) {
			aReachedEnd = true;
			break;
		}
		if (theFrames != nullptr && aSaved.size() == theFrames->size())
			break;
	}

	std::string aUnsupported;
	const std::map<std::string, int>& aSkipped = aMovie.GetUnsupported();
	for (std::map<std::string, int>::const_iterator anItr = aSkipped.begin(); anItr != aSkipped.end(); ++anItr) {
		char aBuf[128];
		snprintf(aBuf, sizeof(aBuf), "%s%s x%d", aUnsupported.empty() ? "" : ", ", anItr->first.c_str(), anItr->second);
		aUnsupported += aBuf;
	}
	std::string aName = thePath.substr(thePath.find_last_of("\\/") + 1);
	std::sort(aTimes.begin(), aTimes.end());
	double aMedian = aTimes.empty() ? 0.0 : aTimes[aTimes.size() / 2];
	double aP95 = aTimes.empty() ? 0.0 : aTimes[aTimes.size() * 95 / 100];
	printf("%s: %dx%d %.0f fps, %d frames, %d ticks, ends at %d; commands: %s; render avg %.2f ms (median %.2f, p95 %.2f) max %.2f ms at frame %d; unsupported: %s\n",
	       aName.c_str(), aMovie.GetWidth(), aMovie.GetHeight(), aMovie.GetFrameRate(), aFrames, aTicks, aMovie.GetCurrentFrame(),
	       aListener.mHeard.empty() ? "none" : aListener.mHeard.c_str(), aTicks ? aTotal / aTicks : 0.0, aMedian, aP95, aMax, aSlowestFrame,
	       aUnsupported.empty() ? "none" : aUnsupported.c_str());
	if (theFrames != nullptr && aSaved.size() != theFrames->size())
		printf("%s: only %d of %d frames were reached\n", aName.c_str(), (int)aSaved.size(), (int)theFrames->size());
	return aReachedEnd || theFrames != nullptr;
}

// Lists what the file defines: every character, or the styles and edges of the one named
int Chars(const std::string& thePath, int theId)
{
	std::vector<uint8_t> aBytes;
	SwfData aData;
	std::string anError;
	if (!Platform::VFS::Get().ReadAll(thePath, aBytes) || !SwfParse(aBytes.data(), aBytes.size(), aData, anError)) {
		printf("%s: FAIL %s\n", thePath.c_str(), anError.c_str());
		return 1;
	}
	static const char* kTypes[] = {"none", "shape", "morph", "sprite", "text", "font"};
	if (theId < 0) {
		// The frames of the movie that have a label or an action
		for (size_t f = 0; f < aData.mRoot.mFrames.size(); f++) {
			const SwfFrame& aFrame = aData.mRoot.mFrames[f];
			if (aFrame.mLabel.empty() && aFrame.mActions.empty())
				continue;
			printf("frame %d:", (int)f);
			if (!aFrame.mLabel.empty())
				printf(" label '%s'", aFrame.mLabel.c_str());
			for (size_t a = 0; a < aFrame.mActions.size(); a++) {
				static const char* kActions[] = {"Play", "Stop", "GetURL"};
				printf(" %s", kActions[aFrame.mActions[a].mType]);
				if (aFrame.mActions[a].mType == SwfAction::kGetURL)
					printf("('%s','%s')", aFrame.mActions[a].mURL.c_str(), aFrame.mActions[a].mTarget.c_str());
			}
			printf("\n");
		}
	}
	std::vector<int> anIds;
	for (std::unordered_map<int, std::unique_ptr<SwfCharacter> >::const_iterator anItr = aData.mChars.begin(); anItr != aData.mChars.end(); ++anItr)
		anIds.push_back(anItr->first);
	std::sort(anIds.begin(), anIds.end());
	for (size_t i = 0; i < anIds.size(); i++) {
		const SwfCharacter& c = *aData.mChars[anIds[i]];
		if (theId >= 0 && anIds[i] != theId)
			continue;
		printf("%d: %s", anIds[i], kTypes[c.mType]);
		if (c.mType == SwfCharacter::kShape) {
			printf(" bounds %.0f,%.0f-%.0f,%.0f, %d fill layers, %d line layers\n", c.mShape.mMinX, c.mShape.mMinY, c.mShape.mMaxX, c.mShape.mMaxY,
			       (int)c.mShape.mFillLayers.size(), (int)c.mShape.mLineLayers.size());
			if (theId >= 0) {
				for (size_t f = 0; f < c.mShape.mFillLayers.size(); f++) {
					const SwfFillLayer& l = c.mShape.mFillLayers[f];
					const SwfFillStyle& s = c.mShape.mFills[l.mStyle];
					printf("  fill %d type 0x%02x color %02x%02x%02x a=%d, %d segs, %d stops", l.mStyle, s.mType, s.mColor.r, s.mColor.g, s.mColor.b, s.mColor.a,
					       (int)l.mSegs.size(), (int)s.mStops.size());
					for (size_t k = 0; k < s.mStops.size(); k++)
						printf(" [%d: %02x%02x%02x a=%d]", s.mStops[k].mRatio, s.mStops[k].mColor.r, s.mStops[k].mColor.g, s.mStops[k].mColor.b, s.mStops[k].mColor.a);
					printf("\n");
				}
			}
		}
		else if (c.mType == SwfCharacter::kSprite)
			printf(" %d frames\n", (int)c.mSprite.mFrames.size());
		else
			printf("\n");
	}
	return 0;
}

// Plays two copies of a movie side by side, one drawn as it is in the game and one drawn whole at every frame, and
// compares them. Returns the number of frames that differ by more than a level or two of color.
int Verify(const std::string& thePath)
{
	SwfMovie aMovies[2];
	TestListener aListeners[2];
	for (int i = 0; i < 2; i++) {
		aListeners[i].mMovie = &aMovies[i];
		aMovies[i].mListener = &aListeners[i];
		if (!aMovies[i].Load(thePath)) {
			printf("%s: FAIL %s\n", thePath.c_str(), aMovies[i].GetError().c_str());
			return 1;
		}
	}
	int w = gWidth > 0 ? gWidth : aMovies[0].GetWidth();
	int h = gHeight > 0 ? gHeight : aMovies[0].GetHeight();
	std::vector<uint32_t> aPixels[2];
	aPixels[0].resize((size_t)w * h);
	aPixels[1].resize((size_t)w * h);
	int aFrames = aMovies[0].GetFrameCount();
	int aBad = 0, aMaxDiff = 0, aFirstBad = -1, aWorst = -1, aCompared = 0;
	size_t aMostPixels = 0;
	for (int aTick = 0; aTick < aFrames * 2; aTick++) {
		for (int i = 0; i < 2; i++)
			aMovies[i].NextFrame();
		aMovies[0].Render(aPixels[0].data(), w, h, w);
		aMovies[1].Invalidate();
		aMovies[1].Render(aPixels[1].data(), w, h, w);
		aCompared++;
		int aFrameMax = 0;
		size_t aCount = 0;
		for (size_t i = 0; i < aPixels[0].size(); i++) {
			uint32_t a = aPixels[0][i], b = aPixels[1][i];
			if (a == b)
				continue;
			int d = std::max(std::max(abs((int)((a >> 16) & 255) - (int)((b >> 16) & 255)), abs((int)((a >> 8) & 255) - (int)((b >> 8) & 255))),
			                 abs((int)(a & 255) - (int)(b & 255)));
			aFrameMax = std::max(aFrameMax, d);
			if (d > 2)
				aCount++;
		}
		if (aFrameMax > 2) {
			aBad++;
			if (aFirstBad < 0)
				aFirstBad = aMovies[0].GetCurrentFrame();
			if (aCount > aMostPixels) {
				aMostPixels = aCount;
				aWorst = aMovies[0].GetCurrentFrame();
			}
		}
		aMaxDiff = std::max(aMaxDiff, aFrameMax);
		if (aMovies[0].GetCurrentFrame() == aFrames - 1)
			break;
	}
	printf("%s: %d frames compared, %d differ (first at frame %d, most pixels %d at frame %d), largest difference %d levels\n", thePath.c_str(), aCompared,
	       aBad, aFirstBad, (int)aMostPixels, aWorst, aMaxDiff);
	return aBad;
}

// Plays to a frame and times a drawing of all of it
int Bench(const std::string& thePath, int theFrame, int theCount)
{
	SwfMovie aMovie;
	TestListener aListener;
	aListener.mMovie = &aMovie;
	aMovie.mListener = &aListener;
	if (!aMovie.Load(thePath)) {
		printf("%s: FAIL %s\n", thePath.c_str(), aMovie.GetError().c_str());
		return 1;
	}
	int w = gWidth > 0 ? gWidth : aMovie.GetWidth();
	int h = gHeight > 0 ? gHeight : aMovie.GetHeight();
	std::vector<uint32_t> aPixels((size_t)w * h);
	for (int aTick = 0; aTick < aMovie.GetFrameCount() * 2; aTick++) {
		aMovie.NextFrame();
		if (aMovie.GetCurrentFrame() >= theFrame)
			break;
	}
	double aTotal = 0, aBest = 1e30;
	for (int i = 0; i < theCount; i++) {
		aMovie.Invalidate();
		auto aStart = std::chrono::steady_clock::now();
		if (gDumpFrame < 0)
			aMovie.Render(aPixels.data(), w, h, w);
		double aMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - aStart).count();
		aTotal += aMs;
		aBest = std::min(aBest, aMs);
	}
	printf("%s frame %d: whole picture %.2f ms on average, %.2f ms at best\n", thePath.c_str(), aMovie.GetCurrentFrame(), aTotal / theCount, aBest);
	return 0;
}

// Checks what the host can do to a movie: Stop, Play, GotoFrame, and what happens at the end of the timeline
int SelfTest(const std::string& thePath)
{
	int aFailures = 0;
	SwfMovie aMovie;
	TestListener aListener;
	aListener.mMovie = &aMovie;
	aMovie.mListener = &aListener;
	if (!aMovie.Load(thePath)) {
		printf("%s: FAIL %s\n", thePath.c_str(), aMovie.GetError().c_str());
		return 1;
	}
	int aLast = aMovie.GetFrameCount() - 1;
	std::vector<uint32_t> aPixels((size_t)aMovie.GetWidth() * aMovie.GetHeight());
	auto Check = [&](bool theOK, const char* theWhat) {
		printf("%s %s\n", theOK ? "PASS" : "FAIL", theWhat);
		if (!theOK)
			aFailures++;
	};

	Check(aMovie.GetCurrentFrame() == 0 && aMovie.IsPlaying(), "before the first tick: frame 0, playing");
	aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == 0, "the first tick shows frame 0");
	Check(aListener.mHeard == "ready@0", "frame 0 tells the host 'ready'");
	for (int i = 0; i < 10; i++)
		aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == 10, "ten ticks on: frame 10");
	aMovie.Stop();
	aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == 10 && !aMovie.IsPlaying(), "Stop: the root timeline stays at frame 10");
	aMovie.Render(aPixels.data(), aMovie.GetWidth(), aMovie.GetHeight(), aMovie.GetWidth());
	aMovie.Render(aPixels.data(), aMovie.GetWidth(), aMovie.GetHeight(), aMovie.GetWidth());
	int aDX, aDY, aDW, aDH;
	Check(!aMovie.GetDamage(aDX, aDY, aDW, aDH), "a second Render without a tick draws nothing");
	aMovie.Invalidate();
	aMovie.Render(aPixels.data(), aMovie.GetWidth(), aMovie.GetHeight(), aMovie.GetWidth());
	Check(aMovie.GetDamage(aDX, aDY, aDW, aDH) && aDX == 0 && aDY == 0 && aDW == aMovie.GetWidth() && aDH == aMovie.GetHeight(),
	      "Invalidate: the next Render draws all of it");
	aMovie.Play();
	aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == 11, "Play: on to frame 11");
	aMovie.GotoFrame(100);
	Check(aMovie.GetCurrentFrame() == 100, "GotoFrame(100) at once");
	aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == 101, "GotoFrame leaves the movie playing");
	aMovie.GotoFrame(40);
	aMovie.Stop();
	aMovie.GotoFrame(60);
	aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == 60, "GotoFrame leaves the movie stopped");
	aMovie.Render(aPixels.data(), aMovie.GetWidth(), aMovie.GetHeight(), aMovie.GetWidth());

	// The end: 'done' is told, the movie stops there, and a Play from the host starts it over
	aListener.mHeard.clear();
	aMovie.Play();
	aMovie.GotoFrame(aLast);
	Check(aMovie.GetCurrentFrame() == aLast && aListener.mHeard.find("done@") == 0, "the last frame tells the host 'done'");
	Check(!aMovie.IsPlaying(), "the last frame stops the movie");
	aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == aLast, "a stopped movie stays at its last frame");
	aMovie.Play();
	aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == 0, "Play at the end: the timeline starts over");
	aMovie.NextFrame();
	Check(aMovie.GetCurrentFrame() == 1, "and goes on from frame 0");

	// TCurrentLabel and TGotoLabel: the movies label each scene "Nstart" and "Nend" (a click skips to the end of the scene)
	aMovie.GotoFrame(0);
	Check(aMovie.GetCurrentLabel() == "1start", "frame 0 is at the label 1start");
	aMovie.GotoFrame(5);
	Check(aMovie.GetCurrentLabel() == "1start", "a frame with no label is at the nearest label before it");
	Check(aMovie.GotoLabel("1end") && aMovie.GetCurrentLabel() == "1end" && aMovie.GetCurrentFrame() > 5,
	      "GotoLabel(1end) jumps to the end of the scene");
	int aFrame = aMovie.GetCurrentFrame();
	Check(!aMovie.GotoLabel("nolabel") && aMovie.GetCurrentFrame() == aFrame, "a label no frame has: no jump");
	return aFailures;
}

std::string GamePath(const std::string& theName)
{
	if (theName.find_first_of("\\/") == std::string::npos)
		return "images\\cutscenes\\" + theName;
	return theName;
}

}  // namespace

int main(int argc, char** argv)
{
	std::vector<std::string> aFiles;
	std::string aRenderFile, aRenderFrames, aRenderPrefix, aDumpFile, aCharsFile, aVerifyFile, aBenchFile;
	int aBenchFrame = 0, aBenchCount = 20;
	bool aSelfTest = false;
	int aCharsId = -1;
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-gamedir=", 9) == 0)
			gGameDir = argv[i] + 9;
		else if (strcmp(argv[i], "-noplay") == 0)
			gAnswerReady = false;
		else if (strncmp(argv[i], "-size=", 6) == 0)
			sscanf(argv[i] + 6, "%dx%d", &gWidth, &gHeight);
		else if (strcmp(argv[i], "-selftest") == 0)
			aSelfTest = true;
		else if (strcmp(argv[i], "-bench") == 0 && i + 2 < argc) {
			aBenchFile = argv[i + 1];
			aBenchFrame = atoi(argv[i + 2]);
			i += 2;
			if (i + 1 < argc && argv[i + 1][0] != '-')
				aBenchCount = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-verify") == 0 && i + 1 < argc)
			aVerifyFile = argv[++i];
		else if (strcmp(argv[i], "-chars") == 0 && i + 1 < argc) {
			aCharsFile = argv[++i];
			if (i + 1 < argc && argv[i + 1][0] != '-')
				aCharsId = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-dump") == 0 && i + 2 < argc) {
			aDumpFile = argv[i + 1];
			gDumpFrame = atoi(argv[i + 2]);
			i += 2;
		}
		else if (strcmp(argv[i], "-render") == 0 && i + 3 < argc) {
			aRenderFile = argv[i + 1];
			aRenderFrames = argv[i + 2];
			aRenderPrefix = argv[i + 3];
			i += 3;
		}
		else if (argv[i][0] == '-') {
			printf("unknown argument: %s\n", argv[i]);
			return 1;
		}
		else
			aFiles.push_back(argv[i]);
	}
	Platform::VFS::Get().SetBaseDir(gGameDir);

	if (aSelfTest)
		return SelfTest(GamePath(aFiles.empty() ? "intro.swf" : aFiles[0]));
	if (!aBenchFile.empty())
		return Bench(GamePath(aBenchFile), aBenchFrame, aBenchCount);
	if (!aVerifyFile.empty())
		return Verify(GamePath(aVerifyFile)) == 0 ? 0 : 1;
	if (!aCharsFile.empty())
		return Chars(GamePath(aCharsFile), aCharsId);
	if (!aDumpFile.empty())
		return Play(GamePath(aDumpFile), nullptr, "") ? 0 : 1;
	if (!aRenderFile.empty()) {
		std::vector<int> aFrames;
		for (size_t p = 0; p < aRenderFrames.size();) {
			aFrames.push_back(atoi(aRenderFrames.c_str() + p));
			size_t aComma = aRenderFrames.find(',', p);
			if (aComma == std::string::npos)
				break;
			p = aComma + 1;
		}
		return Play(GamePath(aRenderFile), &aFrames, aRenderPrefix) ? 0 : 1;
	}

	if (aFiles.empty())
		aFiles = FindMovies();
	else {
		for (size_t i = 0; i < aFiles.size(); i++)
			aFiles[i] = GamePath(aFiles[i]);
	}
	if (aFiles.empty()) {
		printf("no .swf files under %s/images/cutscenes\n", gGameDir.c_str());
		return 1;
	}
	int aFailures = 0;
	for (size_t i = 0; i < aFiles.size(); i++) {
		if (!Play(aFiles[i], nullptr, ""))
			aFailures++;
	}
	return aFailures;
}
