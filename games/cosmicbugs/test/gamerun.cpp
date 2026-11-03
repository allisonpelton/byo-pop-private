// Runs the whole game headless, as the host does (GameStart, then one GameFrame each 10 ms of a clock of its own),
// with input from a script and the screen written as PNGs. Opens no window, plays no sound and opens no URL.
//   cosmicbugs_run <gamedir> <outdir> <script>
//   cosmicbugs_run -same <png> <png>      whether two screenshots have the same pixels (exit 0 when they do)
// The script has one event a line, at a time in ms from the start ('#' starts a comment):
//   <ms> move <x> <y>         the mouse to (x, y)
//   <ms> click <x> <y>        moves there, presses the left button and lets it go 30 ms later
//   <ms> down|up [1|2|3]      a mouse button (1 left, 2 right, 3 middle)
//   <ms> key <vk>             a key (its virtual-key code, decimal or 0x) pressed and let go 30 ms later
//   <ms> hold|release <vk>    a key pressed, or let go
//   <ms> shot <name>          the screen to <outdir>/<name>.png
//   <ms> state                prints the score, the lives, the board in its set and the board's filled cells
//   <ms> end                  stops (the game's end runs)
// The player's files (cp.cfg, cb.dat) are in <outdir>; a missing cp.cfg is made windowed.

#include "R64.h"
#include "R64Sys.h"
#include "CBGlobals.h"
#include "CBLevel.h"
#include "CBGame.h"

#include "stb_image.h"
#include "stb_image_write.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace r64;

namespace
{

struct Event
{
	int time;
	std::string what;
	int a, b;
	std::string name;
};

class RunHost : public SysHost
{
public:
	std::string mDir;
	std::string mOut;
	std::vector<uint16_t> mScreen = std::vector<uint16_t>(800 * 600, 0);
	uint32_t mTime = 0;
	int mMouseX = 400, mMouseY = 300;
	int mSounds = 0;
	std::string mSavePath;

	bool ReadFile(const char* thePath, std::vector<uint8_t>& theData) override
	{
		std::string aPath = mDir + "/" + thePath;
		for (char& c : aPath)
			if (c == '\\')
				c = '/';
		FILE* f = std::fopen(aPath.c_str(), "rb");
		if (f == nullptr)
			return false;
		std::fseek(f, 0, SEEK_END);
		long n = std::ftell(f);
		std::fseek(f, 0, SEEK_SET);
		theData.resize((size_t)n);
		bool ok = n == 0 || std::fread(theData.data(), (size_t)n, 1, f) == 1;
		std::fclose(f);
		return ok;
	}

	// CB_NOWARP=1 ignores the game's warps, as origrun's scripted cursor does for the original's SetCursorPos
	void SetMousePos(int x, int y) override
	{
		if (std::getenv("CB_NOWARP") != nullptr)
			return;
		mMouseX = x;
		mMouseY = y;
	}

	void GetMousePos(int* x, int* y) override
	{
		*x = mMouseX;
		*y = mMouseY;
	}

	bool LockScreen(uint8_t** theBits, int* thePitch) override
	{
		*theBits = (uint8_t*)mScreen.data();
		*thePitch = 1600;
		return true;
	}

	uint32_t TimeMs() override
	{
		return mTime;
	}

	// The sounds load as ids that play nothing
	int LoadSound(const char*) override
	{
		return mSounds++;
	}

	bool LoadMusic(const char* thePath) override
	{
		std::printf("%u ms: music %s\n", mTime, thePath);
		return true;
	}

	void OpenURL(const char* theUrl) override
	{
		std::printf("OpenURL in a test run: %s\n", theUrl);
	}

	const char* SavePath(const char* theName) override
	{
		mSavePath = mOut + "/" + theName;
		return mSavePath.c_str();
	}
};

RunHost gHost;

void WritePng(const std::string& thePath)
{
	std::vector<uint8_t> aRgb(800 * 600 * 3);
	for (int i = 0; i < 800 * 600; i++)
	{
		// widened as the host's widget does, so its screenshots and these compare exactly
		uint16_t p = gHost.mScreen[i];
		int aRed = (p >> 11) & 0x1f, aGreen = (p >> 5) & 0x3f, aBlue = p & 0x1f;
		aRgb[i * 3] = (uint8_t)((aRed << 3) | (aRed >> 2));
		aRgb[i * 3 + 1] = (uint8_t)((aGreen << 2) | (aGreen >> 4));
		aRgb[i * 3 + 2] = (uint8_t)((aBlue << 3) | (aBlue >> 2));
	}
	stbi_write_png(thePath.c_str(), 800, 600, 3, aRgb.data(), 800 * 3);
}

bool ReadScript(const char* thePath, std::vector<Event>& theEvents)
{
	FILE* f = std::fopen(thePath, "r");
	if (f == nullptr)
		return false;
	char aLine[512];
	while (std::fgets(aLine, sizeof(aLine), f) != nullptr)
	{
		char* aHash = std::strchr(aLine, '#');
		if (aHash != nullptr)
			*aHash = 0;
		char aWhat[64] = "", anArg1[256] = "", anArg2[64] = "";
		int aTime;
		int n = std::sscanf(aLine, "%d %63s %255s %63s", &aTime, aWhat, anArg1, anArg2);
		if (n < 2)
			continue;
		Event e;
		e.time = aTime;
		e.what = aWhat;
		e.a = n >= 3 ? (int)std::strtol(anArg1, nullptr, 0) : 0;
		e.b = n >= 4 ? (int)std::strtol(anArg2, nullptr, 0) : 0;
		e.name = anArg1;
		if (e.what == "click")
		{
			theEvents.push_back({aTime, "move", e.a, e.b, ""});
			theEvents.push_back({aTime, "down", 1, 0, ""});
			theEvents.push_back({aTime + 30, "up", 1, 0, ""});
		}
		else if (e.what == "key")
		{
			theEvents.push_back({aTime, "hold", e.a, 0, ""});
			theEvents.push_back({aTime + 30, "release", e.a, 0, ""});
		}
		else
			theEvents.push_back(e);
	}
	std::fclose(f);
	std::stable_sort(theEvents.begin(), theEvents.end(), [](const Event& a, const Event& b) { return a.time < b.time; });
	return true;
}

} // namespace

// -same: whether two screenshots have the same pixels (this runner's and the windowed exe's -script screenshot)
static int ComparePngs(const char* theFirst, const char* theSecond)
{
	int w1, h1, w2, h2, n;
	unsigned char* a = stbi_load(theFirst, &w1, &h1, &n, 3);
	unsigned char* b = stbi_load(theSecond, &w2, &h2, &n, 3);
	int aResult = 2;
	if (a == nullptr || b == nullptr)
		std::printf("cannot read %s\n", a == nullptr ? theFirst : theSecond);
	else if (w1 != w2 || h1 != h2)
		std::printf("sizes differ: %dx%d and %dx%d\n", w1, h1, w2, h2);
	else
	{
		int aDiffer = 0;
		for (int i = 0; i < w1 * h1; i++)
			if (std::memcmp(a + i * 3, b + i * 3, 3) != 0)
				aDiffer++;
		if (aDiffer == 0)
			std::printf("same\n");
		else
			std::printf("%d pixels differ\n", aDiffer);
		aResult = aDiffer == 0 ? 0 : 1;
	}
	stbi_image_free(a);
	stbi_image_free(b);
	return aResult;
}

int main(int argc, char** argv)
{
	if (argc == 4 && std::strcmp(argv[1], "-same") == 0)
		return ComparePngs(argv[2], argv[3]);
	if (argc < 4)
	{
		std::fprintf(stderr, "usage: cosmicbugs_run <gamedir> <outdir> <script>\n       cosmicbugs_run -same <png> <png>\n");
		return 2;
	}
	gHost.mDir = argv[1];
	gHost.mOut = argv[2];
	std::vector<Event> anEvents;
	if (!ReadScript(argv[3], anEvents))
	{
		std::fprintf(stderr, "cannot read %s\n", argv[3]);
		return 2;
	}
	static PixelFormat aFormat;
	PixelFormat_Init(&aFormat, 16, 0xf800, 0x7e0, 0x1f);
	Sys_SetScreenFormat(&aFormat);
	Sys_SetHost(&gHost);
	if (!cb::LoadGameTexts())
		return 1;
	std::string aConfig = gHost.mOut + "/cp.cfg";
	FILE* f = std::fopen(aConfig.c_str(), "rb");
	if (f == nullptr)
	{
		f = std::fopen(aConfig.c_str(), "wb");
		if (f != nullptr)
			std::fprintf(f, "<CP_CONFIG\n    fullscreen=\"0\"\n    desktopResolution=\"0\"\n    wave=\"1\"\n    music=\"1\"\n/>\n");
	}
	if (f != nullptr)
		std::fclose(f);
	LoadConfig();
	Sound_Start();
	cb::GameStart();
	size_t aNext = 0;
	bool aRunning = true;
	int aFrames = 0;
	// CB_WATCH=1: origrun's --watch lines for the seed, the score, the lives and the filled cells, read after each frame
	// (where origrun's virtual clock reads them, at the frame's wait), for comparing a run with the original's
	bool aWatch = std::getenv("CB_WATCH") != nullptr;
	int aWatched[4] = {0, 0, 0, 0};
	bool aKnown[4] = {false, false, false, false};
	while (aRunning)
	{
		while (aNext < anEvents.size() && anEvents[aNext].time <= (int)gHost.mTime)
		{
			const Event& e = anEvents[aNext++];
			if (e.what == "move")
			{
				gHost.mMouseX = e.a;
				gHost.mMouseY = e.b;
			}
			else if (e.what == "down" || e.what == "up")
				Sys_MouseButton(e.a != 0 ? e.a : 1, e.what == "down" ? 1 : 0);
			else if (e.what == "hold" || e.what == "release")
				Sys_KeyEvent(e.a, e.what == "hold" ? 1 : 0);
			else if (e.what == "shot")
				WritePng(gHost.mOut + "/" + e.name + ".png");
			else if (e.what == "state")
			{
				cb::Level* aLevel = cb::gLevels.current;
				std::printf("%u ms: score %d, lives %d, board %d, filled %d of %d%%\n", gHost.mTime, cb::gScore, cb::gLives,
					cb::gLevelInSet, aLevel != nullptr ? aLevel->filledCells : -1, aLevel != nullptr ? aLevel->fillQuota : -1);
			}
			else if (e.what == "end")
				aRunning = false;
		}
		if (!aRunning)
			break;
		if (cb::GameFrame() == 0)
		{
			std::printf("the game quit at %u ms\n", gHost.mTime);
			break;
		}
		aFrames++;
		gHost.mTime += 10;
		if (aWatch)
		{
			cb::Level* aLevel = cb::gLevels.current;
			int aNow[4] = {cb::RandValue(), cb::gScore, cb::gLives, aLevel != nullptr ? aLevel->filledCells : 0};
			for (int i = 0; i < 4; i++)
			{
				if (aKnown[i] && aNow[i] == aWatched[i])
					continue;
				std::printf("watch %d: %u at update %d\n", i + 1, (unsigned)aNow[i], cb::gFrame);
				aWatched[i] = aNow[i];
				aKnown[i] = true;
			}
		}
	}
	cb::GameEnd();
	Sound_Shutdown();
	std::printf("%d frames, %u ms, %d sounds\n", aFrames, gHost.mTime, gHost.mSounds);
	return 0;
}
