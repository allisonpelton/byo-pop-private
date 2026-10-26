// bwa2_pamtest: PopAnim (popanim/) without the game, headless: no window, no sound.
//
//     bwa2_pamtest [-gamedir=<dir>] [-v] [-play] [-draw]
//     bwa2_pamtest [-gamedir=<dir>] [-bg=RRGGBB] [-frac=<0..1>] [-size=<w>x<h>] -render <pam> <frame> <out.png>
//     bwa2_pamtest [-gamedir=<dir>] -info <pam>
//     bwa2_pamtest [-gamedir=<dir>] -run <pam> <label>
//
// With no command it loads every .pam under <dir>/images (default assets/bookwormadv2) and every .txt mod file
// there, prints one line for each that fails and, last, "loaded N of M" (the .pams). Pictures that could not be
// loaded (the game's .jp2 files) do not fail a load; they are listed under -v. -play also plays each animation
// through its main sprite's frames, twice, and -draw draws each at several frames onto a scratch image: both only
// to see that nothing crashes. The exit code is the number of failures.
//
// -render loads <pam> (a path under <dir>, as the game names it), plays it <frame> frames from its start, draws
// it onto a gray background (-bg) the size of its bounds (-size to change that) and writes a PNG. <frame> is a
// number, a label of the animation, or a label and a number of frames after it ("idle+5"). -frac sets how far
// past the frame the animation is, from 0 to 1, to look at the in-between drawing. -info lists what an animation
// holds: its bounds, images, labels and commands. -run plays a label as the game does, one update (10 ms) at a
// time, until the animation stops, and lists the updates at which it told the game something.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include <SDL.h>  // main becomes SDL_main, which the platform's SDL2main calls
#include <stb_image_write.h>

#include "DDInterface.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "PopAnim.h"
#include "SexyAppBase.h"
#include "platform/VFS.h"

using namespace Sexy;

namespace {

std::string gGameDir = "assets/bookwormadv2";
bool gVerbose = false;

// Counts what the animations tell the game
class TestListener : public PopAnimListener {
public:
	int mStops = 0;
	int mCommands = 0;
	int mUpdate = -1;  // the update to name in the lines of -run (-1 when not running)

	void PopAnimStopped(int) override
	{
		mStops++;
		if (mUpdate >= 0)
			printf("update %d: stopped\n", mUpdate);
	}
	void PopAnimCommand(int, const std::string& theCommand, const std::string& theParam) override
	{
		mCommands++;
		if (mUpdate >= 0)
			printf("update %d: command %s %s\n", mUpdate, theCommand.c_str(), theParam.c_str());
		else if (gVerbose)
			printf("    command %s %s\n", theCommand.c_str(), theParam.c_str());
	}
};

// The files under images with an extension, as the game names them (images\pam\lex.pam), sorted
std::vector<std::string> FindFiles(const char* theExt)
{
	std::vector<std::string> aFiles;
	std::filesystem::path aRoot = std::filesystem::path(gGameDir) / "images";
	std::error_code ec;
	for (std::filesystem::recursive_directory_iterator anItr(aRoot, ec), anEnd; !ec && anItr != anEnd; anItr.increment(ec)) {
		if (!anItr->is_regular_file())
			continue;
		std::string aName = anItr->path().filename().string();
		size_t aDot = aName.rfind('.');
		if (aDot == std::string::npos || StringToLower(aName.substr(aDot)) != theExt)
			continue;
		std::string aRel = std::filesystem::relative(anItr->path(), gGameDir, ec).string();
		for (size_t i = 0; i < aRel.size(); i++) {
			if (aRel[i] == '/')
				aRel[i] = '\\';
		}
		aFiles.push_back(aRel);
	}
	std::sort(aFiles.begin(), aFiles.end());
	return aFiles;
}

// Plays the main sprite's frames through, twice, and puts them on a scratch image at a few of them
void Exercise(PopAnim* theAnim, bool thePlay, bool theDraw, MemoryImage* theCanvas)
{
	int aFrames = (int)theAnim->mData->mMain.mFrames.size();
	if (aFrames == 0)
		return;
	theAnim->PlayFrame(0);
	Graphics g(theCanvas);
	for (int i = 0; i < 2 * aFrames + 1; i++) {
		if (theDraw && (i % std::max(1, aFrames / 4) == 0 || i == 2 * aFrames)) {
			g.ClearClipRect();
			theAnim->mFraction = (i % 3) * 0.3f;
			theAnim->Draw(&g);
		}
		if (thePlay) {
			theAnim->mPlaying = true;  // frames that stop the animation do not stop this
			theAnim->NextFrame();
		}
	}
}

int LoadAll(bool thePlay, bool theDraw)
{
	int aFailures = 0;
	TestListener aListener;
	MemoryImage aCanvas;
	aCanvas.Create(800, 600);
	SexyAppBase* anApp = gSexyAppBase;

	std::vector<std::string> aMods = FindFiles(".txt");
	int aModsLoaded = 0;
	for (size_t i = 0; i < aMods.size(); i++) {
		PopAnim* anAnim = new PopAnim(1, &aListener);
		if (anAnim->LoadAnimation(aMods[i]))
			aModsLoaded++;
		else {
			aFailures++;
			printf("FAIL %s: %s\n", aMods[i].c_str(), anAnim->mError.c_str());
		}
		if (gVerbose && anAnim->mLoaded)
			printf("%s: loads %s (%d images)\n", aMods[i].c_str(), anAnim->mPamFileName.c_str(), (int)anAnim->mImages.size());
		delete anAnim;
	}
	anApp->CleanSharedImages();

	std::vector<std::string> aPams = FindFiles(".pam");
	int aLoaded = 0;
	std::set<std::string> aMissing;
	int aFilesMissing = 0;
	int aVersions[4] = {0, 0, 0, 0};
	for (size_t i = 0; i < aPams.size(); i++) {
		PopAnim* anAnim = new PopAnim(1, &aListener);
		if (!anAnim->LoadAnimation(aPams[i])) {
			aFailures++;
			printf("FAIL %s: %s\n", aPams[i].c_str(), anAnim->mError.c_str());
		}
		else {
			aLoaded++;
			aVersions[anAnim->mVersion & 3]++;
			if (!anAnim->mMissingImages.empty()) {
				aFilesMissing++;
				for (size_t m = 0; m < anAnim->mMissingImages.size(); m++)
					aMissing.insert(anAnim->mMissingImages[m]);
				if (gVerbose)
					printf("%s: %d image(s) missing\n", aPams[i].c_str(), (int)anAnim->mMissingImages.size());
			}
			if (thePlay || theDraw)
				Exercise(anAnim, thePlay, theDraw, &aCanvas);
		}
		delete anAnim;
		anApp->CleanSharedImages();
	}
	if (gVerbose) {
		for (std::set<std::string>::iterator anItr = aMissing.begin(); anItr != aMissing.end(); ++anItr)
			printf("missing image: %s\n", anItr->c_str());
	}
	printf("mod files: loaded %d of %d\n", aModsLoaded, (int)aMods.size());
	printf("versions: 0:%d 1:%d 2:%d 3:%d\n", aVersions[0], aVersions[1], aVersions[2], aVersions[3]);
	printf("pictures that could not be loaded: %d, in %d files\n", (int)aMissing.size(), aFilesMissing);
	printf("loaded %d of %d\n", aLoaded, (int)aPams.size());
	return aFailures;
}

int Info(const std::string& thePam)
{
	TestListener aListener;
	PopAnim anAnim(1, &aListener);
	if (!anAnim.LoadAnimation(thePam)) {
		printf("FAIL %s: %s\n", thePam.c_str(), anAnim.mError.c_str());
		return 1;
	}
	printf("%s: version %d, %d frames a second, bounds %d,%d %dx%d\n", anAnim.mPamFileName.c_str(), anAnim.mVersion,
	       anAnim.mFrameRate, anAnim.mPamX, anAnim.mPamY, anAnim.mPamWidth, anAnim.mPamHeight);
	printf("%d images:", (int)anAnim.mImages.size());
	for (size_t i = 0; i < anAnim.mImages.size(); i++) {
		MemoryImage* anImage = anAnim.mImages[i].mImage;
		printf(" %s%s%s", anAnim.mImages[i].mName.c_str(), anAnim.mImages[i].mDrawMode ? "(add)" : "", anImage ? "" : "(missing)");
	}
	printf("\n%d sprites, main sprite %d frames, %d object slots\n", (int)anAnim.mData->mSprites.size(),
	       (int)anAnim.mData->mMain.mFrames.size(), (int)anAnim.mData->mMain.mObjectDefs.size());
	for (std::map<std::string, int>::iterator anItr = anAnim.mData->mMain.mLabels.begin();
	     anItr != anAnim.mData->mMain.mLabels.end(); ++anItr)
		printf("label %s at frame %d\n", anItr->first.c_str(), anItr->second);
	for (size_t f = 0; f < anAnim.mData->mMain.mFrames.size(); f++) {
		const PAFrame& aFrame = anAnim.mData->mMain.mFrames[f];
		if (aFrame.mHasStop)
			printf("frame %d stops\n", (int)f);
		for (size_t c = 0; c < aFrame.mCommands.size(); c++)
			printf("frame %d: command %s %s\n", (int)f, aFrame.mCommands[c].mCommand.c_str(), aFrame.mCommands[c].mParam.c_str());
	}
	return 0;
}

int Run(const std::string& thePam, const std::string& theLabel)
{
	TestListener aListener;
	PopAnim anAnim(1, &aListener);
	if (!anAnim.LoadAnimation(thePam)) {
		printf("FAIL %s: %s\n", thePam.c_str(), anAnim.mError.c_str());
		return 1;
	}
	aListener.mUpdate = 0;
	if (!anAnim.PlayLabel(theLabel)) {
		printf("FAIL %s: no such label: %s\n", thePam.c_str(), theLabel.c_str());
		return 1;
	}
	int aStart = anAnim.mRoot->mFrameNum;
	int anUpdate = 0;
	while (anAnim.IsPlaying() && anUpdate < 100000) {
		aListener.mUpdate = ++anUpdate;
		anAnim.UpdateF(1.0f);
	}
	printf("%s %s: frame %d to %d in %d updates (%d ms)\n", thePam.c_str(), theLabel.c_str(), aStart, anAnim.mRoot->mFrameNum,
	       anUpdate, anUpdate * gSexyAppBase->mFrameTime);
	return 0;
}

bool WritePng(const char* thePath, MemoryImage* theImage)
{
	int w = theImage->mWidth;
	int h = theImage->mHeight;
	const unsigned int* aBits = (const unsigned int*)theImage->GetBits();
	std::vector<unsigned char> aRGBA((size_t)w * h * 4);
	for (int i = 0; i < w * h; i++) {
		unsigned int c = aBits[i];
		aRGBA[i * 4] = (c >> 16) & 0xff;
		aRGBA[i * 4 + 1] = (c >> 8) & 0xff;
		aRGBA[i * 4 + 2] = c & 0xff;
		aRGBA[i * 4 + 3] = (c >> 24) & 0xff;
	}
	return stbi_write_png(thePath, w, h, 4, &aRGBA[0], w * 4) != 0;
}

int Render(const std::string& thePam, const std::string& theFrame, const char* theOut, unsigned int theBg, float theFrac,
           int theWidth, int theHeight)
{
	TestListener aListener;
	PopAnim anAnim(1, &aListener);
	if (!anAnim.LoadAnimation(thePam)) {
		printf("FAIL %s: %s\n", thePam.c_str(), anAnim.mError.c_str());
		return 1;
	}
	for (size_t m = 0; m < anAnim.mMissingImages.size(); m++)
		printf("missing image: %s\n", anAnim.mMissingImages[m].c_str());

	// <frame>: a number, a label, or a label+number
	std::string aLabel = theFrame;
	int aSteps = 0;
	size_t aPlus = theFrame.find('+');
	if (aPlus != std::string::npos) {
		aLabel = theFrame.substr(0, aPlus);
		aSteps = atoi(theFrame.c_str() + aPlus + 1);
	}
	bool aIsNumber = !aLabel.empty() && aLabel.find_first_not_of("0123456789") == std::string::npos;
	bool aStarted;
	if (aIsNumber) {
		aStarted = anAnim.PlayFrame(0);
		aSteps += atoi(aLabel.c_str());
	}
	else
		aStarted = anAnim.PlayLabel(aLabel);
	if (!aStarted) {
		printf("FAIL %s: no such frame or label: %s\n", thePam.c_str(), aLabel.c_str());
		return 1;
	}
	for (int i = 0; i < aSteps; i++) {
		anAnim.mPlaying = true;
		anAnim.NextFrame();
	}
	anAnim.mFraction = theFrac;

	int w = theWidth > 0 ? theWidth : (anAnim.mPamWidth > 0 ? anAnim.mPamWidth : 640);
	int h = theHeight > 0 ? theHeight : (anAnim.mPamHeight > 0 ? anAnim.mPamHeight : 480);
	MemoryImage aCanvas;
	aCanvas.Create(w, h);
	Graphics g(&aCanvas);
	g.SetColor(Color((theBg >> 16) & 0xff, (theBg >> 8) & 0xff, theBg & 0xff));
	g.FillRect(0, 0, w, h);
	g.Translate(-anAnim.mPamX, -anAnim.mPamY);
	anAnim.Draw(&g);
	if (!WritePng(theOut, &aCanvas)) {
		printf("FAIL cannot write %s\n", theOut);
		return 1;
	}
	printf("%s: frame %d of %d, %dx%d, %d images -> %s\n", thePam.c_str(), anAnim.mRoot->mFrameNum,
	       (int)anAnim.mData->mMain.mFrames.size(), w, h, (int)anAnim.mImages.size(), theOut);
	return 0;
}

}  // namespace

int main(int argc, char** argv)
{
	const char* aRenderPam = NULL;
	const char* aRenderFrame = NULL;
	const char* aRenderOut = NULL;
	const char* anInfoPam = NULL;
	const char* aRunPam = NULL;
	const char* aRunLabel = NULL;
	unsigned int aBg = 0x808080;
	float aFrac = 0;
	int aWidth = 0, aHeight = 0;
	bool aPlay = false, aDraw = false;
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-gamedir=", 9) == 0)
			gGameDir = argv[i] + 9;
		else if (strcmp(argv[i], "-v") == 0)
			gVerbose = true;
		else if (strcmp(argv[i], "-play") == 0)
			aPlay = true;
		else if (strcmp(argv[i], "-draw") == 0)
			aDraw = aPlay = true;
		else if (strncmp(argv[i], "-bg=", 4) == 0)
			aBg = (unsigned int)strtoul(argv[i] + 4, NULL, 16);
		else if (strncmp(argv[i], "-frac=", 6) == 0)
			aFrac = (float)atof(argv[i] + 6);
		else if (strncmp(argv[i], "-size=", 6) == 0)
			sscanf(argv[i] + 6, "%dx%d", &aWidth, &aHeight);
		else if (strcmp(argv[i], "-run") == 0 && i + 2 < argc) {
			aRunPam = argv[i + 1];
			aRunLabel = argv[i + 2];
			i += 2;
		}
		else if (strcmp(argv[i], "-info") == 0 && i + 1 < argc)
			anInfoPam = argv[++i];
		else if (strcmp(argv[i], "-render") == 0 && i + 3 < argc) {
			aRenderPam = argv[i + 1];
			aRenderFrame = argv[i + 2];
			aRenderOut = argv[i + 3];
			i += 3;
		}
		else {
			printf("unknown argument: %s\n", argv[i]);
			return 2;
		}
	}

	// The least the framework needs to load images: the game folder, an app (the images belong to one), and a
	// display object (never started: an image in memory draws without it). Never deleted, so nothing at exit
	// waits on a window or the sound.
	Platform::VFS::Get().SetBaseDir(gGameDir);
	SexyAppBase* anApp = new SexyAppBase();
	anApp->mDDInterface = new DDInterface(anApp);

	int aResult;
	if (aRunPam != NULL)
		aResult = Run(aRunPam, aRunLabel);
	else if (anInfoPam != NULL)
		aResult = Info(anInfoPam);
	else if (aRenderPam != NULL)
		aResult = Render(aRenderPam, aRenderFrame, aRenderOut, aBg, aFrac, aWidth, aHeight);
	else
		aResult = LoadAll(aPlay, aDraw);
	fflush(stdout);
	_exit(aResult);
}
