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
//   <ms> focus lose|regain    the window loses or regains the focus (the host's IsActive)
//   <ms> state                prints the score, the lives, the board in its set and the board's filled cells
//   <ms> enemies              prints each enemy of the board: its place, tint and animation
//   <ms> cmd <line>           a standard test command (docs/TEST_COMMANDS.md: state, goto, level, win, move ...), answers printed
//   <ms> bot <ms>             plays by the port's own bot (the move command) for that long, or until the game is over
//   <ms> end                  stops (the game's end runs)
// The player's files (cp.cfg, cb.dat) are in <outdir>; a missing cp.cfg is made windowed.

#include "R64.h"
#include "R64Sys.h"
#include "CBGlobals.h"
#include "CBLevel.h"
#include "CBGame.h"
#include "CBTestCommands.h"

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
	bool mActive = true;

	bool IsActive() override
	{
		return mActive;
	}

	// CB_SKIPLOGOS=1: the site's "Always skip start-up logos", for comparing the draws with and without
	bool SkipLogos() override
	{
		return std::getenv("CB_SKIPLOGOS") != nullptr;
	}

	int mSounds = 0;
	std::string mSavePath;
	bool m555 = std::getenv("CB_555") != nullptr;

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

	// The sounds load as ids that play nothing. CB_SOUNDLOG=1 prints each call of the engine's sound and music, with the
	// update, for comparing with the original's BASS calls (tools/play/soundcmp.py)
	bool mSoundLog = std::getenv("CB_SOUNDLOG") != nullptr;

	void SoundLog(const char* theEvent, const char* theArg = "")
	{
		if (mSoundLog)
			std::printf("sound %d: %s %s\n", cb::gFrame, theEvent, theArg);
	}

	int LoadSound(const char* thePath) override
	{
		SoundLog("load", thePath);
		return mSounds++;
	}

	void PlaySound(int theSound) override
	{
		SoundLog("play", std::to_string(theSound).c_str());
	}

	void StopSounds() override
	{
		SoundLog("stopall");
	}

	bool LoadMusic(const char* thePath) override
	{
		std::printf("%u ms: music %s\n", mTime, thePath);
		SoundLog("musicload", thePath);
		return true;
	}

	void FreeMusic() override
	{
		SoundLog("musicfree");
	}

	void PlayMusic(int theRestart) override
	{
		SoundLog(theRestart != 0 ? "musicplay" : "musicresume");
	}

	void PauseMusic() override
	{
		SoundLog("musicpause");
	}

	void SetMusicVolume(int theVolume) override
	{
		SoundLog("musicvolume", std::to_string(theVolume).c_str());
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
		if (gHost.m555)
		{
			aRed = (p >> 10) & 0x1f;
			aGreen = (p >> 5) & 0x1f;
			aRgb[i * 3] = (uint8_t)((aRed << 3) | (aRed >> 2));
			aRgb[i * 3 + 1] = (uint8_t)((aGreen << 3) | (aGreen >> 2));
		}
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
		if (std::strcmp(aWhat, "cmd") == 0)
		{
			// the rest of the line is the command
			const char* aRest = std::strstr(aLine, "cmd") + 3;
			while (*aRest == ' ' || *aRest == '\t')
				aRest++;
			std::string aCommand = aRest;
			while (!aCommand.empty() && (aCommand.back() == '\n' || aCommand.back() == '\r' || aCommand.back() == ' '))
				aCommand.pop_back();
			theEvents.push_back({aTime, "cmd", 0, 0, aCommand});
			continue;
		}
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
	std::stable_sort(theEvents.begin(), theEvents.end(), [](const Event& a, const Event& b) { return a.mTime < b.mTime; });
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
	static PixelFormat sAFormat;
	// CB_555=1 draws on a 5-5-5 screen, the mode the original gets on this machine (windowed on the host and in Windows
	// Sandbox), for comparing shots with its own
	if (std::getenv("CB_555") != nullptr)
		PixelFormat_Init(&sAFormat, 16, 0x7c00, 0x3e0, 0x1f);
	else
		PixelFormat_Init(&sAFormat, 16, 0xf800, 0x7e0, 0x1f);
	Sys_SetScreenFormat(&sAFormat);
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
	// On this machine the original is handed the 5-5-5 screen after it has loaded, so its screen-lost callback
	// (0x41e6f0) runs once: 0x40d1f0 makes every level's background again from the bank and redraws the screen, which
	// leaves the menus' images and logos blended onto the backgrounds once instead of twice. CB_555 does the same.
	// (Its repacking of palettes and animations changes nothing when the format is already 5-5-5.)
	if (std::getenv("CB_555") != nullptr)
	{
		for (cb::Level* aLevel = cb::gLevels.mHead; aLevel != nullptr; aLevel = cb::Node_Next(aLevel))
		{
			if (aLevel->mBackground == nullptr)
				continue;
			Image_Destroy(aLevel->mBackground);
			::operator delete(aLevel->mBackground);
			aLevel->mBackground = new Image;
			Image_Copy(aLevel->mBackground, Bank_FindImage2(&cb::gArtBank, aLevel->mBackImage));
		}
		cb::Screen_Redraw();
	}
	size_t aNext = 0;
	bool aRunning = true;
	int aFrames = 0;
	// CB_WATCH=1: origrun's --watch lines for the seed, the score, the lives and the filled cells, read after each frame
	// (where origrun's virtual clock reads them, at the frame's wait), for comparing a run with the original's
	// (CB_WATCH=enemies: instead the tints of the board's first eight enemies, as origrun's [[0x473138]+0x41c]+0x40,
	// [[[0x473138]+0x41c]]+0x40 and so on along the list)
	bool aWatch = std::getenv("CB_WATCH") != nullptr;
	bool anEnemies = aWatch && std::strcmp(std::getenv("CB_WATCH"), "enemies") == 0;
	int aCount = anEnemies ? 8 : 4;
	int aWatched[8] = {};
	bool aKnown[8] = {};
	// "bot": plays by the port's own move command, its input steps turned into timed events
	bool aBotOn = false;
	uint32_t aBotEnd = 0;
	uint32_t aBotReady = 0;
	int aBotMoves = 0;
	while (aRunning)
	{
		while (aNext < anEvents.size() && anEvents[aNext].mTime <= (int)gHost.mTime)
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
			else if (e.mWhat == "focus")
				gHost.mActive = e.mName == "regain";
			else if (e.mWhat == "shot")
				WritePng(gHost.mOut + "/" + e.mName + ".png");
			else if (e.mWhat == "state")
			{
				cb::Level* aLevel = cb::gLevels.mCurrent;
				std::printf("%u ms: score %d, lives %d, board %d, filled %d of %d%%\n", gHost.mTime, cb::gScore, cb::gLives,
					cb::gLevelInSet, aLevel != nullptr ? aLevel->mFilledCells : -1, aLevel != nullptr ? aLevel->mFillQuota : -1);
			}
			else if (e.mWhat == "enemies")
			{
				std::printf("enemies at %u ms\n", gHost.mTime);
				for (cb::Enemy* anEnemy = cb::gLevels.mCurrent->mEnemies.mHead; anEnemy != nullptr; anEnemy = cb::Node_Next(anEnemy))
					std::printf("enemy at %.1f,%.1f tint %d anim %s\n", anEnemy->mX, anEnemy->mY, anEnemy->mTint,
						anEnemy->mAnim != nullptr ? anEnemy->mAnim->mName : "-");
			}
			else if (e.mWhat == "cmd")
			{
				std::string aName = e.mName;
				std::string anArgs;
				size_t aSpace = aName.find(' ');
				if (aSpace != std::string::npos)
				{
					anArgs = aName.substr(aSpace + 1);
					aName = aName.substr(0, aSpace);
				}
				std::vector<std::string> aReplies;
				if (!cb::TestCommand(aName, anArgs, aReplies))
					aReplies.push_back("ERROR " + e.mName + ": unknown command");
				for (const std::string& aLine : aReplies)
					std::printf("%u ms: %s\n", gHost.mTime, aLine.c_str());
			}
			else if (e.mWhat == "bot")
			{
				aBotOn = true;
				aBotEnd = gHost.mTime + (uint32_t)e.mA;
				aBotReady = gHost.mTime;
			}
			else if (e.mWhat == "end")
				aRunning = false;
		}
		if (!aRunning)
			break;
		if (aBotOn && gHost.mTime >= aBotEnd)
		{
			aBotOn = false;
			std::printf("%u ms: bot stopped by time after %d moves\n", gHost.mTime, aBotMoves);
		}
		else if (aBotOn && gHost.mTime >= aBotReady)
		{
			std::vector<std::string> aReplies;
			cb::TestCommand("state", "", aReplies);
			if (aReplies.back().find("\"over\":true") != std::string::npos)
			{
				aBotOn = false;
				std::printf("%u ms: bot stopped, the game is over after %d moves\n", gHost.mTime, aBotMoves);
			}
			else
			{
				aReplies.clear();
				cb::TestCommand("move", "", aReplies);
				const std::string& aMove = aReplies.back();
				int aTime = (int)gHost.mTime;
				if (aMove.compare(0, 9, "MOVE null") == 0)
					aBotReady = gHost.mTime + 100;
				else
				{
					aBotMoves++;
					size_t anAt = aMove.find("\"input\":[");
					size_t anEnd = aMove.size();
					while (anAt < anEnd)
					{
						size_t anOpen = aMove.find('{', anAt + 1);
						if (anOpen == std::string::npos)
							break;
						size_t aClose = aMove.find('}', anOpen);
						if (aClose == std::string::npos)
							break;
						std::string o = aMove.substr(anOpen, aClose - anOpen + 1);
						anAt = aClose;
						int x = 0, y = 0, n = 0;
						size_t k;
						if ((k = o.find("\"move\":[")) != std::string::npos && std::sscanf(o.c_str() + k + 8, "%d,%d", &x, &y) == 2)
							anEvents.push_back({aTime, "move", x, y, ""});
						else if ((k = o.find("\"click\":[")) != std::string::npos && std::sscanf(o.c_str() + k + 9, "%d,%d", &x, &y) == 2)
						{
							int aButton = o.find("\"right\"") != std::string::npos ? 2 : 1;
							anEvents.push_back({aTime, "move", x, y, ""});
							anEvents.push_back({aTime, "down", aButton, 0, ""});
							anEvents.push_back({aTime + 30, "up", aButton, 0, ""});
							aTime += 40;
						}
						else if ((k = o.find("\"wait\":")) != std::string::npos && std::sscanf(o.c_str() + k + 7, "%d", &n) == 1)
							aTime += n;
					}
					std::stable_sort(anEvents.begin() + (long)aNext, anEvents.end(),
						[](const Event& a, const Event& b) { return a.mTime < b.mTime; });
					aBotReady = (uint32_t)aTime + 10;
				}
			}
		}
		if (cb::GameFrame() == 0)
		{
			std::printf("the game quit at %u ms\n", gHost.mTime);
			break;
		}
		aFrames++;
		gHost.mTime += 10;
		if (aWatch)
		{
			cb::Level* aLevel = cb::gLevels.mCurrent;
			int aNow[8] = {cb::RandValue(), cb::gScore, cb::gLives, aLevel != nullptr ? aLevel->mFilledCells : 0};
			if (anEnemies)
			{
				cb::Enemy* anEnemy = aLevel != nullptr ? aLevel->mEnemies.mHead : nullptr;
				for (int i = 0; i < 8; i++)
				{
					aNow[i] = anEnemy != nullptr ? anEnemy->mTint : -1;
					anEnemy = anEnemy != nullptr ? cb::Node_Next(anEnemy) : nullptr;
				}
			}
			for (int i = 0; i < aCount; i++)
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
