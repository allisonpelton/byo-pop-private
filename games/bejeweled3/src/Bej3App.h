// Sexy::Bej3App (vtable 0x8647b4 (bejeweled3), 105 slots): Bejeweled 3's app. Its fields' comments start with their
// offsets in the original's object; the framework's own fields are SexyAppBase's. docs/NOTES.md ("Boot") lists what
// the original's Init and loading thread do, and which of it is translated yet.
#pragma once

#include "SexyApp.h"

#include <map>
#include <string>
#include <vector>

namespace Sexy {

class Bej3Music;
class Board;
class MemoryImage;
class MainMenu;
class PrimeResourceManager;
class Profile;
class QuestDataParser;
class QuestMenu;

class Bej3App : public SexyApp {
public:
	// The three sizes of the game's art (images\1200\, images\768\, images\600\): the height of the screen it was
	// drawn for. The 2D mode always uses 600 at 800x600.
	enum { ART_RES_1200 = 1200, ART_RES_768 = 768, ART_RES_600 = 600 };

	// +0x7f0: the original's SexyAppBase::mResourceManager, which in this framework generation is the manifest-2 manager
	// (engines/prime); the framework's own mResourceManager is left unused
	PrimeResourceManager* mPrimeResourceManager = nullptr;
	int mArtRes = ART_RES_600;              // +0x9e8: "ArtRes"
	bool mArtResFromCmdLine = false;        // +0x9dc: -600, -768 or -1200 chose the art size: "ArtRes" is not read
	bool mArtResChanged = false;            // +0x9db: the options changed the art size: "ArtRes" is written
	double mVoiceVolume = 0.85;             // +0x990: "VoiceVolume", in percent in the registry
	double mZenAmbientVolume = 0.5;         // +0x998: "ZenAmbientVolume"
	double mZenAmbientMusicVolume = 0.0;    // +0x9a0: "ZenAmbientMusicVolume"
	double mZenBinauralVolume = 0.25;       // +0x9a8: "ZenBinauralVolume"
	double mZenBreathVolume = 0.5;          // +0x9b0: "ZenBreathVolume"
	bool mAnimateBackground = true;         // +0x9e0: "AnimateBackground"
	int mTipIndex = 0;                      // +0xd78: "TipIdx", the loading screen's next tip in properties\tips.txt
	Profile* mProfile = nullptr;            // +0xb80: the players and the one loaded
	MainMenu* mMainMenu = nullptr;          // +0xba0: the first screen
	Bej3Music* mBej3Music = nullptr;        // +0xa30: the same object as mMusicInterface, with the game's songs (null without sound)
	QuestMenu* mQuestMenu = nullptr;        // +0xbd8: the quest map, while it is up (0x773b70 makes it)
	Board* mBoard = nullptr;                // +0xbe8: the game screen (made by the mode's start, 0x7737d0 for Classic)
	// +0xb5c: the gems' sprites, made by 0x76bfc0 from IMAGE_GEMS_SHADOWED: a row of 15 for each colour (the frame is picked by
	// the gem's size). port: made when the first board is made, not at the end of loading
	MemoryImage* mGemFrames[7][15] = {};
	void MakeGemFrames();                   // 0x76bfc0 (bejeweled3)
	bool mSavedGameChosen = false;          // +0xc31: set when a mode's start resumed a saved game; cleared before a mode starts
	QuestDataParser* mDefaultQuestData = nullptr;  // +0xbc8: properties\defaultquest.cfg
	QuestDataParser* mQuestData = nullptr;  // +0xbcc: properties\quest.cfg
	QuestDataParser* mSecretQuestData = nullptr;   // +0xbd0: properties\secret.cfg
	QuestDataParser* mSpeedQuestData = nullptr;    // +0xbd4: properties\speed.cfg
	std::vector<std::string> mTips;         // +0xcf0: properties\tips.txt, a tip a line (the loading screen shows them)
	std::vector<std::string> mRankNames;    // +0xd08: properties\ranks.txt, a name a line
	std::vector<std::string> mBadgeNames;   // +0xd20: properties\badges.txt: the part of each line before the colon
	// +0xd38 and +0xd50: the numbers after the colon, split at '/' into one entry for each badge level; an entry's
	// first number is its goal and its second (after a ',') is 0 when the line has none
	std::vector<std::vector<int>> mBadgeGoals;
	std::vector<std::vector<int>> mBadgeSecondGoals;
	// port: the 3D mode comes later (docs/PORT_PLAN.md, "Rendering"): until then every run is the original's 2D mode
	bool mUse3D = false;

	Bej3App();                              // 0x76d9e0 (bejeweled3)
	~Bej3App() override;

	void Init() override;                   // 0x7748e0 (bejeweled3)
	MusicInterface* CreateMusicInterface() override;  // 0x76cd70 (bejeweled3)
	void LoadingThreadProc() override;      // 0x772740 (bejeweled3)
	void WriteToRegistry() override;        // 0x76d450 (bejeweled3)
	void ReadFromRegistry() override;       // 0x76fe40 (bejeweled3)
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;  // 0x76f9d0 (bejeweled3)
	void MakeWindow() override;
	void PostDDInterfaceInitHook() override;
	SharedImageRef GetSharedImage(const std::string& theFileName, const std::string& theVariant = "",
		bool* isNew = nullptr) override;    // 0x46ff40 (bejeweled3)

	// 0x76b020 (bejeweled3): without 3D the game is 800x600 with the 600 art
	void Use2DMode();

	// 0x76bc30 (bejeweled3): the four quest files; each that cannot be read shows its error
	void LoadQuestData();
	// 0x7748e0 (bejeweled3): the text files tips.txt, ranks.txt and badges.txt
	void LoadTextFiles();
	// 0x7748e0 (bejeweled3): the players, and the last one chosen (the registry's "LastUser") or the first that loads
	void LoadProfile();

	// 0x7737d0 (bejeweled3): starts Classic: the board of the last game goes, a new ClassicBoard is made as big as the
	// screen, put on it with the focus, its first game dealt, and the game's music starts. port: the saved game that
	// 0x76cde0 asks about is none
	void StartClassic();
	void StartZen();         // 0x773100 (bejeweled3)
	void StartLightning();   // 0x773440 (bejeweled3)
	// 0x774060 (bejeweled3): starts a secret mode, the quest at the index of secret.cfg (0 Poker, 1 Butterflies; 2 and 3 are not made); false when
	// that quest's mode is not made
	bool StartSecretMode(int theIndex);
	// port: test only (the `quest` command): settings that replace the quest's own in the next StartQuest
	std::map<std::string, std::string> mQuestSettingChanges;
	// 0x774060 (bejeweled3) with the secret flag 0: starts the quest at an index of quest.cfg (the quest map's buttons); false when that quest's challenge is not made
	bool StartQuest(int theIndex);
	// 0x773b70 (bejeweled3): the quest map is shown (the play menu's QUEST ball, a lost or won quest); with theDeleteBoard the board of the game goes
	void ShowQuestMenu(bool theDeleteBoard);
	// the quest map's BACK (0x697640's id 2): the map and the board under it go and the play menu comes back
	void LeaveQuestMenu();
	// the quest map is up
	bool IsQuestMenuVisible() const { return mQuestMenu != nullptr; }
	void StartBoard(Board* theBoard);   // the shared end of the starts

	// 0x7716e0 (bejeweled3): the game screen is leaving: the play menu (the screen PLAY opens) comes back and the menu's song plays
	void ShowPlayMenu();
	// port: the standard test commands (docs/TEST_COMMANDS.md): seed, state, goto classic
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	// port: test only (the `name` command). A name typed into the new user dialog and its OK pressed, as a player does: at once when the
	// dialog is open, else when the welcome dialog opens (a game with a player already has no such dialog and the command does nothing)
	std::string mScriptName;
	// port: test only (the `play` and `unlock` commands): the mode and quest the bot's `move` clicks its way to from the menus (a hand's route), and an
	// `unlock all` that came before the first player existed
	std::string mPlayTarget;
	int mPlayQuest = 0;
	bool mUnlockPending = false;
	// port: test only (the `atmove` command): a test command that `move` runs once the board has had that many moves, to shorten a long mode
	bool mIdle = false;   // the `idle` command: `move` gives no swap any more (a player who walked away), so a mode that ends on its own clock ends
	int mSwapsGiven = 0;   // the swaps `move` has answered with so far (what `atmove` counts)
	int mAtMoveCount = -1;
	std::string mAtMoveCommand;
	std::string mAtMoveArgs;
	void EnterScriptName();

	// 0x76aaf0 (bejeweled3): plays a sound of the sound manager at a volume, pan and pitch (steps); nothing when muted
	void PlaySoundEx(int theSoundId, double theVolume, int thePan, double thePitch);

	// 0x76e4a0 (bejeweled3): the game's dialogs are Bej3Dialogs
	Dialog* NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
		const SexyString& theDialogFooter, int theButtonMode) override;
	// 0x771a70 (bejeweled3): what a dialog's button does. port: only the new user dialog (id 1) and the closing every
	// dialog ends with are made; the user list, rename, delete and the other screens' dialogs come with their screens
	void DialogButtonDepress(int theDialogId, int theButtonId) override;
	// 0x771040 (bejeweled3): the welcome dialog, asking the first player's name
	void DoWelcomeDialog();

	// The framework's two, on this generation's manager
	void LoadResourceManifest();            // 0x46e0e0 (bejeweled3)
	void ShowResourceError(bool doExit = false);  // 0x46e190 (bejeweled3)
};

extern Bej3App* gApp;

}  // namespace Sexy
