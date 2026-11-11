// Runs the whole game headless, as the host does (GameStart, then one GameFrame each 10 ms of a clock of its own),
// with input from a script and the screen written as PNGs. Opens no window, plays no sound and opens no URL.
//   waterbugs_run <gamedir> <outdir> <script>
//   waterbugs_run -same <png> <png>      whether two screenshots have the same pixels (exit 0 when they do)
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
#include "WBGlobals.h"
#include "WBLevel.h"

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
	int mTime;
	std::string mWhat;
	int mA, mB;
	std::string mName;
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
		bool anOk = n == 0 || std::fread(theData.data(), (size_t)n, 1, f) == 1;
		std::fclose(f);
		return anOk;
	}

	// WB_USER: the Windows user's name, which the game greets with (the original run in the sandbox reads "SYSTEM")
	void UserName(char* theName, int theSize) override
	{
		const char* aName = std::getenv("WB_USER");
		if (theSize > 0)
			std::snprintf(theName, (size_t)theSize, "%s", aName != nullptr ? aName : "");
	}

	// WB_NOWARP: the game's own cursor moves are ignored, as in the sandbox where the original's SetCursorPos has no
	// effect and the pointer stays where the harness put it
	void SetMousePos(int x, int y) override
	{
		if (std::getenv("WB_NOWARP") != nullptr)
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
		e.mTime = aTime;
		e.mWhat = aWhat;
		e.mA = n >= 3 ? (int)std::strtol(anArg1, nullptr, 0) : 0;
		e.mB = n >= 4 ? (int)std::strtol(anArg2, nullptr, 0) : 0;
		e.mName = anArg1;
		if (e.mWhat == "click")
		{
			theEvents.push_back({aTime, "move", e.mA, e.mB, ""});
			theEvents.push_back({aTime, "down", 1, 0, ""});
			theEvents.push_back({aTime + 30, "up", 1, 0, ""});
		}
		else if (e.mWhat == "key")
		{
			theEvents.push_back({aTime, "hold", e.mA, 0, ""});
			theEvents.push_back({aTime + 30, "release", e.mA, 0, ""});
		}
		else
			theEvents.push_back(e);
	}
	std::fclose(f);
	std::stable_sort(theEvents.begin(), theEvents.end(),
		[](const Event& a, const Event& b) { return a.mTime < b.mTime; });
	return true;
}

} // namespace

// -same: whether two screenshots have the same pixels (this runner's and the windowed exe's -script screenshot)
static int ComparePngs(const char* theFirst, const char* theSecond)
{
	int aW1, aH1, aW2, aH2, n;
	unsigned char* a = stbi_load(theFirst, &aW1, &aH1, &n, 3);
	unsigned char* b = stbi_load(theSecond, &aW2, &aH2, &n, 3);
	int aResult = 2;
	if (a == nullptr || b == nullptr)
		std::printf("cannot read %s\n", a == nullptr ? theFirst : theSecond);
	else if (aW1 != aW2 || aH1 != aH2)
		std::printf("sizes differ: %dx%d and %dx%d\n", aW1, aH1, aW2, aH2);
	else
	{
		int aDiffer = 0;
		for (int i = 0; i < aW1 * aH1; i++)
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
		std::fprintf(stderr, "usage: waterbugs_run <gamedir> <outdir> <script>\n       waterbugs_run -same <png> <png>\n");
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
	static PixelFormat sAFormat;
	PixelFormat_Init(&sAFormat, 16, 0xf800, 0x7e0, 0x1f);
	Sys_SetScreenFormat(&sAFormat);
	Sys_SetHost(&gHost);
	if (!wb::LoadGameTexts())
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
	wb::GameStart();
	size_t aNext = 0;
	bool aRunning = true;
	bool aByFrame = std::getenv("WB_BY_FRAME") != nullptr;
	int aFrames = 0;
	while (aRunning)
	{
		// (WB_BY_FRAME: the script's times are the game's update counter, not the clock: the original's captures are
		// taken at a counter value)
		int aNow = aByFrame ? wb::gFrame : (int)gHost.mTime;
		while (aNext < anEvents.size() && anEvents[aNext].mTime <= aNow)
		{
			const Event& e = anEvents[aNext++];
			if (e.mWhat == "move")
			{
				gHost.mMouseX = e.mA;
				gHost.mMouseY = e.mB;
			}
			else if (e.mWhat == "down" || e.mWhat == "up")
				Sys_MouseButton(e.mA != 0 ? e.mA : 1, e.mWhat == "down" ? 1 : 0);
			else if (e.mWhat == "hold" || e.mWhat == "release")
				Sys_KeyEvent(e.mA, e.mWhat == "hold" ? 1 : 0);
			else if (e.mWhat == "shot")
				WritePng(gHost.mOut + "/" + e.mName + ".png");
			else if (e.mWhat == "state")
			{
				wb::Level* aLevel = wb::gLevels.mCurrent;
				std::printf("%u ms: score %d, lives %d, board %d, filled %d of %d%%\n", gHost.mTime, wb::gScore,
					wb::gLives, wb::gLevelInSet, aLevel != nullptr ? aLevel->mFilledCells : -1,
					aLevel != nullptr ? aLevel->mFillQuota : -1);
				// the update counter (0x460a1c), for lining up with the original
				std::printf("frame %d, %d random draws, seed 0x%x\n", wb::gFrame, wb::gRandCalls,
					(unsigned)wb::RandValue());
					wb::Shooter* aShip = aLevel != nullptr ? aLevel->mShooters.mCurrent : nullptr;
					if (aShip != nullptr)
						std::printf("ship %d,%d state %d age %d mouse %d,%d\n", (int)aShip->mX, (int)aShip->mY,
							aShip->mState, (int)aShip->mAge, wb::gMouseX, wb::gMouseY);
			}
			else if (e.mWhat == "lives")
				wb::gLives = e.mA;  // a test shortcut: the lives left, to reach the game over
			else if (e.mWhat == "score")
				wb::gScore = e.mA;  // a test shortcut: a score for the high-score entry
			else if (e.mWhat == "reached")
				wb::gProfiles[wb::gProfile].mLevelReached = e.mA;  // a test shortcut: levels finished, so a game can be
																   // resumed
			else if (e.mWhat == "die" && wb::gLevels.mCurrent != nullptr &&
				wb::gLevels.mCurrent->mShooters.mCurrent != nullptr)
			{
				// a test shortcut: an enemy's hit on the ship (Beam_UpdateRider's), to reach the game over
				wb::Shooter_Hit(wb::gLevels.mCurrent->mShooters.mCurrent, 1.0f, 1.0f);
				wb::gLives = wb::gLives - 1;
			}
			else if (e.mWhat == "end")
				aRunning = false;
		}
		if (!aRunning)
			break;
		if (wb::GameFrame() == 0)
		{
			std::printf("the game quit at %u ms\n", gHost.mTime);
			break;
		}
		aFrames++;
		// (a frame is WB_FRAME_MS ms, 10 unless set: the original under origrun runs about a millisecond a frame. It is
		// shortened to reach an event that falls inside it: the game steps a millisecond at a time)
		static const uint32_t FRAME_MS = std::getenv("WB_FRAME_MS") != nullptr ?
			(uint32_t)std::atoi(std::getenv("WB_FRAME_MS")) : 10;
		uint32_t aStep = FRAME_MS;
		int aAfter = aByFrame ? wb::gFrame : (int)gHost.mTime;   // (the frame just run has moved the counter on)
		if (aNext < anEvents.size() && anEvents[aNext].mTime > aAfter && anEvents[aNext].mTime < aAfter + (int)FRAME_MS)
			aStep = (uint32_t)(anEvents[aNext].mTime - aAfter);
		gHost.mTime += aStep;
	}
	wb::GameEnd();
	Sound_Shutdown();
	std::printf("%d frames, %u ms, %d sounds\n", aFrames, gHost.mTime, gHost.mSounds);
	return 0;
}
