#include "BookWormApp.h"
#include "Compat2004.h"
#include "Board.h"
#include "Replay.h"
#include "platform/ModManager.h"
#include "StatKeeper.h"
#include "TitleScreen.h"
#include "BWGameSelector.h"
#include "HelpWidget.h"
#include "FameWidget.h"
#include "LevelUpWidget.h"
#include "Definitionator.h"
#include "OptionsDialog.h"
#include "CreditsDialog.h"
#include "NewUserDialog.h"
#include "SelectUserDialog.h"
#include "DeleteUserDialog.h"
#include "HintDialog.h"
#include "GameText.h"
#include "ModFeatures.h"
#include "ModWords.h"
#include "LiveSplitObserver.h"
#include "SidePanel.h"
#include "testhooks/Autoplay.h"

#include "WidgetManager.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "Insets.h"
#include "Buffer.h"
#include "SoundManager.h"
#include "MusicInterface.h"
#include "SDLMusicInterface.h"
#include "PakInterface.h"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

using namespace Sexy;

BookWormApp* Sexy::gApp = NULL;
MTRand* Sexy::gGameRand = NULL;

// 0x52e410
const char* const Sexy::gImageNames[NUM_IMAGES] =
{
	"backdrop_title", "backdrop_title_loadmeter", "backdrop_register", "logo_title", "title_loadmeter",
	"title_stamp_trial", "title_stamp_registered", "dialog", "dialogbutton", "button_reg_register",
	"button_reg_cancel", "button_reg_regcode", "editbox", "cursor_pointer", "cursor_hand",
	"cursor_dragging", "cursor_text", "backdrop_board", "backdrop_board_scorched", "backdrop_bonus",
	"backdrop_menu", "backdrop_fame", "backdrop_help", "backdrop_help_box", "backdrop_ihs",
	"backdrop_wb_speech", "backdrop_wb_thought", "backdrop_wb_text", "gameselect_action",
	"gameselect_action_over", "gameselect_classic", "gameselect_classic_over", "gameselect_hall",
	"gameselect_hall_over", "gameselect_quit", "gameselect_quit_over", "gameselect_bubble",
	"dialog_levelup", "button_fame_continue", "button_help_continue", "button_goto_ihs",
	"button_goto_fame", "button_submit_ihs", "button_submit_ihs_hilite", "button_help_topic",
	"button_help_topic_hilite", "ihs_tab_names", "ihs_tab_names_hilite", "ihs_tab_indicator",
	"ihs_alias_warning", "bar_levelup_header", "bar_gameover_header", "bar_footer_scroll",
	"bar_footer_continue", "rankstrip", "rankframe", "level_up_star", "fx_ashflake", "fx_ember_sm",
	"fx_ember_med", "fx_ember_lrg", "fx_ember_smoke_sm", "fx_ember_smoke_med", "fx_ember_smoke_lrg",
	"fx_particle_sm", "fx_particle_med", "fx_particle_lrg", "fx_spark_sm", "fx_spark_med",
	"fx_spark_lrg", "fx_smoke_sm", "fx_flame_lrg", "fx_fireline", "fx_lex_sweat_seq",
	"fx_lex_sweat_trickle", "tile", "tile_hilite", "tile_fire", "tile_fire_hilite", "tile_gold",
	"tile_gold_hilite", "tile_green", "tile_green_hilite", "tile_sapphire", "tile_sapphire_hilite",
	"tile_diamond", "tile_diamond_hilite", "tile_charred", "tile_gemhilite", "tile_spin_fx", "tile_x",
	"font_tile_letters", "font_tile_digits", "tile_green_glow", "tile_gold_glow", "tile_fire_glow",
	"tile_fire_fx1", "tile_fire_fx2", "arrows", "bonusarrow_left", "bonusarrow_right", "bonusarrow_top",
	"lex_shadow", "lex_headless", "lex_glow_idle", "lex_smile_idle", "lex_smile_blink", "lex_giggle_1",
	"lex_giggle_2", "lex_slurp_1", "lex_speak_ah", "lex_speak_ee", "lex_speak_oh", "lex_speak_pb",
	"lex_sweat", "lex_sweat_facing", "lex_burnt", "lex_burnt_blink", "lex_eating", "lexbig_speak_ah",
	"lexbig_speak_ee", "lexbig_speak_mm", "lexbig_speak_oo", "lexbig_speak_pb", "wb_text_shout_tail",
	"wb_text_talk_tail", "wb_text_think_tail", "progress_green", "progress_lavender", "progress_lime",
	"progress_magenta", "progress_multi", "progress_navy", "progress_orange", "progress_purple",
	"progress_red", "progress_tan", "progress_teal", "progress_yellow", "progress_burnt", "sparkle",
	"sparkle_large", "dialogbutton_plain", "slidertrack", "sliderthumb", "checkboxes", "checkboxes_help",
	"waitbar", "clouds"
};

// 0x52e3c0
const char* const Sexy::gFontNames[NUM_FONTS] =
{
	"Verdana10Bold.txt", "Verdana12Bold.txt", "Verdana14Bold.txt", "Verdana18BoldColor.txt",
	"Lockergnome14Outline.txt", "Verdana18Bold.txt", "ScrippsCollegeOS13Bold.txt",
	"ScrippsCollegeOS15Bold.txt", "ScrippsCollegeOS18Bold.txt", "Lockergnome18.txt", "Lockergnome24.txt",
	"CourierFinalDraft20Bold.txt", "GoldScore.txt", "GoldScoreHeated.txt", "LockergnomeSmallColor.txt",
	"LegacySerifMdSCITCTT12.txt", "MyriadWeb10.txt", "MyriadCondensedWeb11.txt", "MyriadWeb12Bold.txt"
};

// 0x52e668
const char* const Sexy::gSoundNames[NUM_SOUNDS] =
{
	"buttonclick", "gameovertone", "bad", "blank", "up1", "up2", "up3", "up4", "up5", "up6", "up7", "up8",
	"up9", "nextselect", "wordcomplete", "bonuscomplete", "buzzer", "piece_hit", "chomp", "flametile",
	"warning", "tileburn", "appear2", "goldtile", "buttonclick", "redtilecleared", "spinsuperbonus",
	"bonusword", "screenburngameover", "lex_astonishing", "lex_awesome", "lex_burp", "lex_careful",
	"lex_danger", "lex_excellent", "lex_fantastic", "lex_getready", "lex_go", "lex_incredible",
	"lex_letsgo", "lex_levelup", "lex_lookout", "lex_scramble", "lex_unbelievable", "lex_verygood",
	"lex_watchout", "lex_welcome"
};

// The default high-score rows (AddDefaultHighScores): 16-byte records {name, word, score, 0}. port: the
// name and word are game texts (GT_NONE: no word)
struct DefaultHighScore
{
	GameTextId		mName;
	GameTextId		mWord;
	int				mScore;
};

// 0x4fa4e8
static const DefaultHighScore gDefaultClassicScores[BookWormApp::NUM_CLASSIC_SCORES] =
{
	{ GT_HS_NAME_WORDPIG, GT_NONE, 5000 }, { GT_HS_NAME_SENHEN, GT_NONE, 4500 }, { GT_HS_NAME_ORCUS, GT_NONE, 4000 },
	{ GT_HS_NAME_GRAVYTRAIN, GT_NONE, 3500 }, { GT_HS_NAME_TUCO, GT_NONE, 3000 }, { GT_HS_NAME_BRENNXOR, GT_NONE, 2500 },
	{ GT_HS_NAME_ALLY, GT_NONE, 2000 }, { GT_HS_NAME_ZELDA, GT_NONE, 1500 }, { GT_HS_NAME_GREENMAN, GT_NONE, 1000 },
	{ GT_HS_NAME_DELIVERATOR, GT_NONE, 42 }
};

// 0x4fa588
static const DefaultHighScore gDefaultActionScores[BookWormApp::NUM_ACTION_SCORES] =
{
	{ GT_HS_NAME_DANIELLEE, GT_NONE, 5000 }, { GT_HS_NAME_DANSMOM, GT_NONE, 4500 },
	{ GT_HS_NAME_ARCHITEKT, GT_NONE, 4000 }, { GT_HS_NAME_JAN, GT_NONE, 3500 }, { GT_HS_NAME_PAMELA, GT_NONE, 3000 },
	{ GT_HS_NAME_ALIEN, GT_NONE, 2500 }, { GT_HS_NAME_ZAK, GT_NONE, 2000 }, { GT_HS_NAME_WORDWEASEL, GT_NONE, 1500 },
	{ GT_HS_NAME_CRAZYHORSE, GT_NONE, 1000 }, { GT_HS_NAME_MRILLITERATE, GT_NONE, 500 }
};

// 0x4fa628
static const DefaultHighScore gDefaultLongestWords[BookWormApp::NUM_WORD_SCORES] =
{
	{ GT_HS_NAME_WORDPIG, GT_HS_WORD_DESIGN, 0 }, { GT_HS_NAME_DELIVERATOR, GT_HS_WORD_CODE, 0 },
	{ GT_HS_NAME_SENHEN, GT_HS_WORD_ART, 0 }
};

// 0x4fa658
static const DefaultHighScore gDefaultBestWords[BookWormApp::NUM_WORD_SCORES] =
{
	{ GT_HS_NAME_GRAVYTRAIN, GT_HS_WORD_PLAY, 300 }, { GT_HS_NAME_ORCUS, GT_HS_WORD_POPCAP, 200 },
	{ GT_HS_NAME_WORDPIG, GT_HS_WORD_GAMES, 100 }
};

// The "players" folder of the saved games (a per-TU static std::string in the original, 0x52e818)
static const char* const gPlayersDir = "players";

//////////////////////////////////////////////////////////////////////////
// Math (Math.cpp 0x42e330-0x42e3d0)

// 0x42e330: the tables are computed in x87 extended precision and stored as floats; the port computes
// them in double precision (nobody reads them).
Math::Math()
{
	mPi = 3.1415925f; // 0x40490fda
	for (int i = 0; i < 360; i++)
	{
		mSin[i] = (float) sin((double) mPi * (double) i * 0.005555555555555556);
		mNegCos[i] = (float) -cos((double) i * (double) mPi * 0.005555555555555556);
	}
}

// 0x42e390
Math::~Math()
{
}

//////////////////////////////////////////////////////////////////////////
// High-score predicates

// 0x4115d0
bool HighScoreByScore::operator()(const HighScoreEntry& a, const HighScoreEntry& b) const
{
	if (a.mScore != b.mScore)
		return a.mScore > b.mScore;
	return strcmp(a.mWord.c_str(), b.mWord.c_str()) < 0;
}

// 0x411570
bool HighScoreByWordLength::operator()(const HighScoreEntry& a, const HighScoreEntry& b) const
{
	if (a.mWord.size() != b.mWord.size())
		return a.mWord.size() > b.mWord.size();
	return a.mScore > b.mScore;
}

//////////////////////////////////////////////////////////////////////////
// Free helpers

// 0x410090
void Sexy::DrawShadowedString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor,
	bool theShadow, const Color& theShadowColor)
{
	if (theShadow)
	{
		// (the original also makes a dead Color::GetAlpha call here)
		g->SetColor(theShadowColor);
		g->DrawString(theString, theX + 1, theY + 1);
	}
	g->SetColor(theColor);
	g->DrawString(theString, theX, theY);
}

// 0x411220: after every image load. The original logs "<name> - PALLETIZED!" or "<name> - 32 Bit Only"
// (OutputDebugString) and, for a 32-bit DDImage, sets mWantDDSurface when 3D is on or else calls
// GenerateDDSurface. port: Palletize only (lossless: it keeps 32-bit images that have more than 256
// colours; the port has no DirectDraw surfaces).
void Sexy::OptimizeImage(Image* theImage, const SexyString& theName)
{
	(void) theName;
	MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	if (aMemoryImage != NULL)
		aMemoryImage->Palletize();
}

// 0x4112f0: CRT toupper (C locale: only a-z change)
void Sexy::UpperCaseString(SexyString& theString)
{
	for (size_t i = 0; i < theString.length(); i++)
	{
		char c = theString[i];
		if ((c >= 'a') && (c <= 'z'))
			theString[i] = (char) (c - ('a' - 'A'));
	}
}

// 0x4122e0
void Sexy::TrimString(SexyString& theString, const SexyString& theChars)
{
	size_t anEnd = theString.find_last_not_of(theChars) + 1; // npos + 1 = 0: everything goes
	if (anEnd > theString.length())
		theString.append(anEnd - theString.length(), '\0'); // unreachable
	else
		theString.erase(anEnd);
	theString.erase(0, theString.find_first_not_of(theChars));
}

// 0x412da0: the longest prefix of theString that fits (whole string first), plus the ellipsis if cut
SexyString Sexy::TruncateToWidth(Graphics* g, const SexyString& theString, int theMaxWidth, const SexyString& theEllipsis)
{
	unsigned int aLength = (unsigned int) theString.length();
	SexyString aResult = theString;
	while ((g->GetFont()->StringWidth(aResult) > theMaxWidth) && ((int) aLength > 0))
	{
		aLength--;
		aResult = theString.substr(0, aLength);
	}
	if ((int) aLength < (int) theString.length())
		aResult += theEllipsis;
	return aResult;
}

//////////////////////////////////////////////////////////////////////////
// BookWormApp

// 0x41a0f0
BookWormApp::BookWormApp()
{
	gApp = this; // port: WinMain stored it (0x5347d4)

	// port: the 2004 SexyApp ctor reads these from the exe (version resource, build-info block
	// "00000190Thu Apr 15 14:52:08 2004"); saved games carry them (version 5 header)
	mProductVersion = "1.03";
	mBuildNum = 190;
	mBuildDate = "Thu Apr 15 14:52:08 2004";

	mUnusedString2 = "";
	mInitDone = false;
	mMusicInterface = NULL;
	for (int i = 0; i < NUM_FONTS; i++)
		mFonts[i] = NULL;
	for (int i = 0; i < NUM_TILESETS; i++)
		mTileSetImages[i] = NULL;
	for (int i = 0; i < 4; i++)
		mUnusedImages[i] = NULL;
	mSelectedGameMode = -1;
	mCheckboxOnImage = NULL;
	mCheckboxOffImage = NULL;
	mImages = NULL;
	mTileSapphireImage = NULL;
	mTileDiamondImage = NULL;
	mRankStripImage = NULL;
	mGameSelector = NULL;
	mBoard = NULL;
	mHelpWidget = NULL;
	mFameWidget = NULL;
	mMusicVolume = 0.5;
	mDefinitionator = NULL;
	mProdName = "BookWorm";
	mTitle = "BookWorm Deluxe " + mProductVersion;
	mRegKey = "PopCap\\BookWorm";
	mCustomCursorsEnabled = false;
	mGameMode = GAMEMODE_CLASSIC;
	mScoringMethod = 1;
	mUserTimesPlayed = 0;
	mScrambled = false;
	mWelcomePlayed = false;
	mInternetOK = false;
	mUnused12eb = false;
	mWatchOutHintShown = false;
	mFireAlertHintShown = false;
	mGreenTileHintShown = false;
	mUnused12ef = false;
	mScrambleHintShown = false;
	for (int i = 0; i < 11; i++)
		mUnusedHintFlags[i] = false;
	mResumeUpdateCnt = 0;
	mLevelReached = false;
	mPlaysToCount = 0;
	mPlayTicks = 0;
	mMaxExecutions = 0;
	mMaxPlays = 0;
	mMaxTime = 0;
	mGameOption = 1;
	mUnused12e6 = true;
	mShowHelp = true;
	mShowHints = true;
	mShowDefs = true;
	mAutoEnable3D = true;

	// port: the original leaves these two uninitialised (Init / ReadFromRegistry set them before any read)
	mTitleScreen = NULL;
	mUserManager = NULL;

	// port: WinMain sets the window size before Init (the port's default is the same)
	mWidth = 640;
	mHeight = 480;

	// port: the test hooks (see BookWormApp.h)
	mAutoplay = false;
	mAutoplaySeed = 0;
	mAutoplayMode = "classic";
	mAutoplayCap = 200000;
	mAutoplayShotWords = 25;
	mAutoplayHints = false;
	mAutoplayUndo = false;
	mAutoplayModel = false;
	mAutoplayContinue = false;
	mAutoplayType = false;
	mAutoplayBot = NULL;
	mDailyDate = 0;
	mSidePanel = NULL;
	mReplayDir = "replays";
	mWatchFile = NULL;
	mWatchSpeed = 1;
	mWatchQuit = false;
	mWatchSavedMode = GAMEMODE_CLASSIC;
	for (int i = 0; i < 5; i++)
		mWatchSavedFlags[i] = false;
	mWatchSavedLetterIdx = -1;
	mWatchSavedRefillLevel = -1;
}

// 0x4187b0. The widgets were removed and deleted by ShutdownWidgets (the help widget is not: original).
BookWormApp::~BookWormApp()
{
	delete mAutoplayBot; // port: the -autoplay test hook
	delete mWatchFile;   // port(mod)
	LiveSplitObserver::AppDestroyed(this); // port(mod): the LiveSplit block's last write

	if (mUserManager != NULL)
	{
		delete mUserManager;
		mUserManager = NULL;
	}

	for (int i = 0; i < NUM_FONTS; i++)
		delete mFonts[i];
	for (int i = 0; i < NUM_TILESETS; i++)
		delete mTileSetImages[i];
	for (int i = 0; i < 4; i++)
		delete mUnusedImages[i];
	delete mCheckboxOnImage;
	delete mCheckboxOffImage;
	for (std::list<MemoryImage*>::iterator anItr = mCopiedImages.begin(); anItr != mCopiedImages.end(); ++anItr)
		delete *anItr;

	if (mImages != NULL)
	{
		for (int i = 0; i < NUM_IMAGES; i++)
			delete mImages[i];
		delete[] mImages;
	}

	delete mDefinitionator;

	// port: WinMain deleted the game generator after the app (D17)
	delete gGameRand;
	gGameRand = NULL;
	if (gApp == this)
		gApp = NULL;
}

// 0x410b30 slot 3: every dialog button press forwarded to the app clicks
void BookWormApp::ButtonPress(int theId)
{
	(void) theId;
	PlaySample(SOUND_BUTTONCLICK);
}

// 0x417080 slot 4: the dialog buttons (Dialog2004: YES / OK / footer = 2000 + dialog id, NO / CANCEL =
// 3000 + dialog id) and the hint dialog's disable-hints button.
void BookWormApp::ButtonDepress(int theId)
{
	// HintDialog's disable-hints button (the dialog listens to it itself, so this is not reached from it;
	// kept)
	if (theId == BUTTON_HINT_DISABLE)
	{
		KillDialog(DIALOG_HINT);
		mShowHints = false;
		return;
	}

	int aRemainder = theId % 10000;
	if ((aRemainder >= 2000) && (aRemainder < 3000))
	{
		// YES of dialog theId - 2000 (the full id: 12008 -> 10008)
		int aDialogId = theId - 2000;
		switch (aDialogId)
		{
		case DIALOG_UNUSED_0:
			KillDialog(DIALOG_UNUSED_0);
			ShowGameSelector();
			return;

		case DIALOG_OPTIONS:
			((OptionsDialog*) GetDialog(DIALOG_OPTIONS))->ApplyAndClose();
			return;

		case DIALOG_REGISTER:
			// not ported: registration (validates the name and code of the RegisterDialog, then a thank-you
			// message); dialog 2 is never created in the registered build
			return;

		case DIALOG_CONTINUE:
			// Continue the saved game: a new Board (through the selector), then the save is loaded and,
			// being single-use, deleted
			KillDialog(DIALOG_CONTINUE);
			mGameSelector->StartGame();
			mBoard->LoadGame(mSelectedGameMode);
			mBoard->DeleteSavedGame(mGameMode);
			return;

		case DIALOG_INTERNET_REQUIRED:
			KillDialog(DIALOG_INTERNET_REQUIRED);
			mInternetOK = true;
			// not ported: FameWidget::StartNetTransfer (internet high scores)
			return;

		case DIALOG_ASK_UPDATE_CHECK:
			KillDialog(DIALOG_ASK_UPDATE_CHECK);
			// 0x415de0 DoUpdateCheck not ported (update check)
			return;

		case DIALOG_QUIT:
			KillDialog(DIALOG_QUIT);
			Shutdown();
			return;

		case DIALOG_REGISTER_UPSELL:
			KillDialog(DIALOG_REGISTER_UPSELL);
			// fall through: ShowRegisterDialog
		case DIALOG_TRIAL_EXPIRED:
			ShowRegisterDialog();
			return;

		case DIALOG_SELECT_USER:
			KillDialog(DIALOG_SELECT_USER);
			return;

		case DIALOG_DELETE_USER:
			// The DeleteUserDialog opens the confirmation (21) itself
			KillDialog(DIALOG_CONFIRM_DELETE_USER);
			return;

		case DIALOG_CONFIRM_DELETE_USER:
		{
			KillDialog(DIALOG_CONFIRM_DELETE_USER);
			DeleteUserDialog* aDialog = (DeleteUserDialog*) GetDialog(DIALOG_DELETE_USER);
			if (aDialog != NULL)
				aDialog->DeleteSelectedUser();
			KillDialog(DIALOG_DELETE_USER);
			return;
		}

		case DIALOG_NEW_USER:
		{
			NewUserDialog* aDialog = (NewUserDialog*) GetDialog(DIALOG_NEW_USER);
			SexyString aName = aDialog->mNameEdit->mString;
			TrimString(aName, " \t\r\n");
			if (aName.length() == 0)
			{
				DoDialog(DIALOG_MESSAGE, true, GameText(GT_ENTER_YOUR_NAME), GameText(GT_ENTER_NAME_LINES),
					GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
				return;
			}
			if (mUserManager->FindUser(aName) != NULL)
			{
				DoDialog(DIALOG_MESSAGE, true, GameText(GT_NAME_CONFLICT_HEADER), GameText(GT_NAME_CONFLICT_LINES),
					GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
				return;
			}

			KillDialog(DIALOG_NEW_USER);
			if (mGameSelector != NULL)
				mGameSelector->SetDisabled(false);
			// The new player keeps the app's current options. SaveUser runs before the User is in the list,
			// so it only makes the profile's key (its values are found by name: UserManager::SaveUser)
			User* aUser = new User(aName);
			mUserTimesPlayed = 0;
			mUserManager->SaveUser(aUser);
			mUserManager->mUsers.push_back(aUser);
			mUserManager->SetCurrentUser(aUser->mName);
			KillDialog(DIALOG_SELECT_USER);
			return;
		}

		case DIALOG_ALIAS:
			// not ported: the internet alias (SetCurrentAlias from the GetStringDialog, then
			// ConfirmInternetConnect when the hall of fame is up)
			KillDialog(DIALOG_ALIAS);
			return;

		case DIALOG_NAME_TAKEN:
			KillDialog(DIALOG_NAME_TAKEN);
			// 0x414080 ShowAliasDialog(GetCurrentAlias()) not ported (internet high scores)
			return;

		case DIALOG_CONNECTING:
			// not ported: FameWidget's mNetState = 0 (internet high scores)
			KillDialog(DIALOG_CONNECTING);
			return;

		case DIALOG_LEAVE_GAME:
			KillDialog(DIALOG_LEAVE_GAME);
			if ((mBoard != NULL) && (mBoard->IsInProgress()))
				mBoard->SaveGame(mGameMode);
			ShowGameSelector();
			return;

		case DIALOG_UPDATE_AVAILABLE:
			KillDialog(DIALOG_UPDATE_AVAILABLE);
			KillDialog(DIALOG_UPDATE_CHECK);
			// not ported: OpenURL(mInternetManager->GetUpdateURL(), true) (update check)
			return;

		case DIALOG_UPDATE_NONE:
			KillDialog(DIALOG_UPDATE_NONE);
			KillDialog(DIALOG_UPDATE_CHECK);
			return;

		default:
			KillDialog(aDialogId);
			return;
		}
	}
	else if ((aRemainder >= 3000) && (aRemainder < 4000))
	{
		// NO of dialog theId - 3000
		int aDialogId = theId - 3000;
		switch (aDialogId)
		{
		case DIALOG_OPTIONS:
			((OptionsDialog*) GetDialog(DIALOG_OPTIONS))->ApplyAndClose();
			return;

		case DIALOG_TRIAL_EXPIRED:
			Shutdown();
			return;

		case DIALOG_CONTINUE:
			// New game: the saved one is deleted
			KillDialog(DIALOG_CONTINUE);
			mGameSelector->StartGame();
			mBoard->DeleteSavedGame(mGameMode);
			mBoard->mState = BOARD_PLAYING;
			return;

		case DIALOG_NEW_USER:
			// Cancel: a player is mandatory
			if (mUserManager->GetCurrentUserName() == "")
			{
				DoDialog(DIALOG_MESSAGE, true, GameText(GT_ENTER_YOUR_NAME), GameText(GT_ENTER_NAME_LINES),
					GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
				return;
			}
			KillDialog(DIALOG_NEW_USER);
			{
				Dialog* aDialog = GetDialog(DIALOG_SELECT_USER);
				if (aDialog != NULL)
					aDialog->Resize(aDialog->mX, aDialog->mY, 400, aDialog->GetPreferredHeight(400));
			}
			if (mGameSelector != NULL)
				mGameSelector->SetDisabled(false);
			return;

		case DIALOG_UPDATE_AVAILABLE:
			KillDialog(DIALOG_UPDATE_AVAILABLE);
			KillDialog(DIALOG_UPDATE_CHECK);
			return;

		default:
			KillDialog(aDialogId);
			return;
		}
	}
}

// 0x4100f0 slot 7: the music fades run twice per update (the base ends with mMusicInterface->Update())
void BookWormApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	// port(mod): replays (Replay.h): a -replay watch starts once the title has loaded, as if its play link were
	// clicked; a faster watch runs whole updates, so Lex and the overlay keep step with the board; -replayquit ends
	// the run at the watch's end
	if ((!mWatchPath.empty()) && (mTitleScreen != NULL) && (mTitleScreen->mLoadDone) && (!mShutdown))
		TitleScreenDone();
	for (int i = 1; (i < mWatchSpeed) && (mBoard != NULL) && (mBoard->mPlayer != NULL) && (!mShutdown); i++)
		SexyApp::UpdateFrames();
	if ((mWatchQuit) && (mBoard != NULL) && (mBoard->mPlayer != NULL) && (mBoard->mPlayer->mEnded) && (!mShutdown))
		Shutdown();
	mMusicInterface->Update();

	// port: the -autoplay test hook (see BookWormApp.h): the bot looks at the game after each update
	if ((mAutoplay) && (mAutoplayBot == NULL))
		mAutoplayBot = new AutoplayBot(this, mAutoplaySeed, mAutoplayMode, mAutoplayCap, mAutoplayLogFileName,
			mAutoplayShotPrefix, mAutoplayShotWords, mAutoplayHints, mAutoplayUndo, mAutoplayModel, mAutoplayType);
	if (mAutoplayBot != NULL)
		mAutoplayBot->Update();

	// port(mod): the LiveSplit block, after the update (LiveSplitObserver.h)
	LiveSplitObserver::AfterUpdate(this);
}

// port: the game's command-line test hooks (see BookWormApp.h); everything else goes to the framework
void BookWormApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplaySeed = strtoul(theParamValue.c_str(), NULL, 10);
		// the bot's input only, as in a -script run (from the start: focus changes would pause the game)
		mScriptedInput = true;
	}
	else if (theParamName == "-autoplaymode")
		mAutoplayMode = theParamValue;
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLogFileName = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShotPrefix = theParamValue;
	else if (theParamName == "-autoplayshotwords")
		mAutoplayShotWords = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplayhints")
		mAutoplayHints = atoi(theParamValue.c_str()) != 0;
	else if (theParamName == "-autoplayundo")
		mAutoplayUndo = atoi(theParamValue.c_str()) != 0;
	else if (theParamName == "-autoplaymodel")
		mAutoplayModel = atoi(theParamValue.c_str()) != 0;
	else if (theParamName == "-autoplaycontinue")
		mAutoplayContinue = atoi(theParamValue.c_str()) != 0;
	else if (theParamName == "-autoplaytype")
		mAutoplayType = atoi(theParamValue.c_str()) != 0;
	else if (theParamName == "-dailydate")
		mDailyDate = atoi(theParamValue.c_str());
	else if (theParamName == "-replay")
		mWatchPath = theParamValue;     // port(mod): replays (Replay.h)
	else if (theParamName == "-replaydir")
		mReplayDir = theParamValue;
	else if (theParamName == "-replayspeed")
	{
		int aSpeed = atoi(theParamValue.c_str());
		mWatchSpeed = (aSpeed >= 8) ? 8 : (aSpeed >= 4) ? 4 : (aSpeed >= 2) ? 2 : 1;
	}
	else if (theParamName == "-replayquit")
		mWatchQuit = atoi(theParamValue.c_str()) != 0;
	else if (theParamName == "-livesplitlog")
		LiveSplitObserver::SetLogFile(theParamValue); // port(mod): test hook, one line per block write
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

// 0x4169d0 slot 13: `if (!GetBoolean("DontUpdate", true) && ShouldCheckForUpdate()) AskForUpdateCheck()`;
// not ported (update check; DontUpdate defaults to true and partner.xml sets it)
void BookWormApp::LoadingThreadCompleted()
{
}

// port: SexyAppBase::Init runs this first, then ReadFromRegistry (LoadHighScores adds the default rows, which
// are game texts), then the window and BookWormApp::Init (the title screen). The texts come from the game
// folder's Bookworm.exe; properties/texts.xml, when present (a mod or language pack), overrides any of them
// by id.
void BookWormApp::InitPropertiesHook()
{
	SexyApp::InitPropertiesHook();

	LoadProperties("properties/texts.xml", false, false);
	if (!LoadGameTexts())
		SDL_Log("BookWorm: no game texts read from Bookworm.exe (texts without an override stay empty)");
}

// port(mod): with the mod "widescreen" the screen is 854 x 480: every screen of the game keeps its 640 x 480 at
// the left (GAME_SCREEN_WIDTH) and Init adds the side panel to the right of it. SexyAppBase::Init resizes the
// widget manager and makes the window after this.
void BookWormApp::PreDisplayHook()
{
	SexyApp::PreDisplayHook();
	if (SidePanel::Enabled())
	{
		mWidth = GAME_SCREEN_WIDTH + SIDE_PANEL_WIDTH;
		SidePanel::TellPage(mWidth, mHeight);
	}
}

// 0x414a10 slot 16
void BookWormApp::Init()
{
	SexyApp::Init();

	// port: the 2004 SexyApp::Init seeds the C library generator with srand(GetTickCount())
	// (Compat2004.h); -seed=N test runs seed it with N, as they do the framework's Rand(), so a run is
	// reproducible (the game's own generators are seeded from Rand())
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());

	if (!CheckRegistryAccess())
	{
		Shutdown();
		return;
	}

	mMaxExecutions = GetInteger("MaxExecutions", 0);
	mMaxPlays = GetInteger("MaxPlays", 0);
	mMaxTime = GetInteger("MaxTime", 60);

	SetMusicVolume(mMusicVolume);

	mImages = new Image*[NUM_IMAGES];
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;

	for (int i = 0; i < NUM_INIT_IMAGES; i++)
	{
		if (!LoadImage(i))
		{
			Shutdown();
			return;
		}
		OptimizeImage(mImages[i], gImageNames[i]);
	}

	for (int i = 0; i < NUM_INIT_FONTS; i++)
	{
		if (!LoadFont(i))
		{
			Shutdown();
			return;
		}
	}

	// A missing sound ends the game silently
	static SexyString gSoundPath = GetString("SoundPath", "sounds\\");
	for (int i = 0; i < NUM_SOUNDS; i++)
	{
		if ((!mSoundManager->LoadSound(i, gSoundPath + gSoundNames[i])) || (mShutdown))
		{
			Shutdown();
			return;
		}
	}

	SetCursorImage(CURSOR_POINTER, mImages[IMAGE_CURSOR_POINTER]);
	SetCursorImage(CURSOR_HAND, mImages[IMAGE_CURSOR_HAND]);
	SetCursorImage(CURSOR_DRAGGING, mImages[IMAGE_CURSOR_DRAGGING]);
	SetCursorImage(CURSOR_TEXT, mImages[IMAGE_CURSOR_TEXT]);
	mInitDone = true;

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, GAME_SCREEN_WIDTH, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);
	if (mWidth > GAME_SCREEN_WIDTH)
	{
		mSidePanel = new SidePanel(this);    // port(mod): the mod "widescreen"
		mSidePanel->Resize(GAME_SCREEN_WIDTH, 0, mWidth - GAME_SCREEN_WIDTH, mHeight);
		mWidgetManager->AddWidget(mSidePanel);
	}

	// WinMain (0x40fe80): the game generator gets one draw of the framework's generator, right after Init
	// (port: here, NAMES.md D17), seeded the 2004 way
	gGameRand = new MTRand();
	SeedMTRand2004(*gGameRand, (unsigned long) Sexy::Rand());
}

// 0x41a8a0 slot 17 (runs synchronously in the port): music, the remaining fonts and images, the derived
// images, the word lists and the definitions. mCompletedLoadingThreadTasks drives the title's bar.
void BookWormApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = NUM_LOADING_TASKS;

	for (int i = 0; i < NUM_SONGS; i++)
	{
		mMusicInterface->LoadMusic(i, "music\\Bookworm1.mo3");
		mCompletedLoadingThreadTasks++;
	}
	// The title theme starts while loading
	mMusicInterface->PlayMusic(SONG_MENU, ORDER_MENU, false);

	for (int i = NUM_INIT_FONTS; i < NUM_FONTS; i++)
	{
		if ((!LoadFont(i)) || (mShutdown))
		{
			mLoadingFailed = true;
			return;
		}
		mCompletedLoadingThreadTasks++;
	}

	for (int i = NUM_INIT_IMAGES; i < NUM_IMAGES; i++)
	{
		if ((!LoadImage(i)) || (mShutdown))
		{
			mLoadingFailed = true;
			return;
		}
		OptimizeImage(mImages[i], gImageNames[i]);
		mCompletedLoadingThreadTasks++;
	}

	// (the original also reads GetString("SoundPath", "sounds\\") here, unused)
	// The 47 sound tasks are counted only without a sound device: with one the bar ends at 166 of 216
	if (!mSoundManager->Initialized())
		mCompletedLoadingThreadTasks += NUM_SOUNDS;

	// The checkbox halves: left unchecked, right checked
	Image* aCheckboxes = mImages[IMAGE_CHECKBOXES];
	int aCheckboxHeight = aCheckboxes->GetHeight();
	int aCheckboxWidth = aCheckboxes->GetWidth() / 2;
	mCheckboxOffImage = CopyImage(aCheckboxes, Rect(0, 0, aCheckboxWidth, aCheckboxHeight));
	mCheckboxOnImage = CopyImage(aCheckboxes, Rect(aCheckboxWidth, 0, aCheckboxWidth, aCheckboxHeight));

	// The tile sets: per tile kind, the 26 letter tiles A..Z side by side, each the background with its
	// letter (grey level per set) and its value in the corner
	for (int aSet = 0; aSet < NUM_TILESETS; aSet++)
	{
		Image* aTileImage = mImages[IMAGE_TILE];
		Image* aBackImage = aTileImage;
		int aLetterGrey = 255;
		switch (aSet)
		{
		case TILESET_NORMAL:		aLetterGrey = 0; break;
		case TILESET_NORMAL_HILITE:	aLetterGrey = 255; aBackImage = mImages[IMAGE_TILE_HILITE]; break;
		case TILESET_GREEN:			aBackImage = mImages[IMAGE_TILE_GREEN]; aLetterGrey = 0; break;
		case TILESET_GREEN_HILITE:	aLetterGrey = 255; aBackImage = mImages[IMAGE_TILE_GREEN_HILITE]; break;
		case TILESET_GOLD:			aBackImage = mImages[IMAGE_TILE_GOLD]; aLetterGrey = 0; break;
		case TILESET_GOLD_HILITE:	aLetterGrey = 176; aBackImage = mImages[IMAGE_TILE_GOLD_HILITE]; break;
		case TILESET_FIRE:			aBackImage = mImages[IMAGE_TILE_FIRE]; aLetterGrey = 0; break;
		case TILESET_FIRE_HILITE:	aLetterGrey = 255; aBackImage = mImages[IMAGE_TILE_FIRE_HILITE]; break;
		case TILESET_CHARRED:		aBackImage = mImages[IMAGE_TILE_CHARRED]; aLetterGrey = 176; break;
		}

		int aSetWidth = aTileImage->GetWidth();
		int aSetHeight = mImages[IMAGE_TILE]->GetHeight();
		MemoryImage* aSetImage = new MemoryImage(this);
		mTileSetImages[aSet] = aSetImage;
		aSetImage->Create(aSetWidth * 26, aSetHeight);

		Graphics g(aSetImage);
		for (int aLetter = 0; aLetter < 26; aLetter++)
		{
			int aX = aBackImage->GetWidth() * aLetter;
			g.DrawImage(aBackImage, aX, 0);
			g.SetColorizeImages(true);

			Image* aLetters = mImages[IMAGE_FONT_TILE_LETTERS];
			Image* aDigits = mImages[IMAGE_FONT_TILE_DIGITS];
			int aLetterHeight = aLetters->GetHeight() / 26;
			g.SetColor(Color(aLetterGrey, aLetterGrey, aLetterGrey));
			g.DrawImage(aLetters, aX, 0, Rect(0, aLetterHeight * aLetter, aLetters->GetWidth(), aLetterHeight));

			// 'Q' is the "Qu" tile: 10 points
			int aValue = (aLetter == 'Q' - 'A') ? 10 : gLetterValues[aLetter];
			int aDigitHeight = aDigits->GetHeight() / 10;
			g.SetColor(Color(255, 255, 255, 255));
			g.DrawImage(aDigits, (aX - aDigits->GetWidth()) + 54, 53 - aDigitHeight,
				Rect(0, (aValue - 1) * aDigitHeight, aDigits->GetWidth(), aDigitHeight));
			g.SetColorizeImages(false);
		}

		OptimizeImage(aSetImage, StrFormat("Tile set %d", aSet));
		mCompletedLoadingThreadTasks++;
	}

	if (!LoadDictionary("wordlist.txt", &mDictionary))
	{
		mLoadingFailed = true;
		return;
	}
	LoadModWords(this, &mDictionary);  // port(mod): the mods' word lists
	mCompletedLoadingThreadTasks++;

	LoadBonusWords("bonus_word_dictionary.txt", &mBonusWords);
	mCompletedLoadingThreadTasks++;

	mDefinitionator = new Definitionator(this);
	LoadModDefinitions(mDefinitionator);  // port(mod): the mods' definitions, the game's replacing them
	mDefinitionator->Load("popdefs.txt");
	mCompletedLoadingThreadTasks++;

	// Copies of three images (the rank strip's original is then deleted)
	if (mTileSapphireImage == NULL)
	{
		Image* aSource = mImages[IMAGE_TILE_SAPPHIRE];
		mTileSapphireImage = new MemoryImage(this);
		mTileSapphireImage->Create(aSource->GetWidth(), aSource->GetHeight());
		Graphics g(mTileSapphireImage);
		g.DrawImage(aSource, 0, 0);
		mCopiedImages.push_back(mTileSapphireImage);
	}
	mCompletedLoadingThreadTasks++;

	if (mTileDiamondImage == NULL)
	{
		Image* aSource = mImages[IMAGE_TILE_DIAMOND];
		mTileDiamondImage = new MemoryImage(this);
		mTileDiamondImage->Create(aSource->GetWidth(), aSource->GetHeight());
		Graphics g(mTileDiamondImage);
		g.DrawImage(aSource, 0, 0);
		mCopiedImages.push_back(mTileDiamondImage);
	}
	mCompletedLoadingThreadTasks++;

	if (mRankStripImage == NULL)
	{
		Image* aSource = mImages[IMAGE_RANKSTRIP];
		mRankStripImage = new MemoryImage(this);
		mRankStripImage->Create(aSource->GetWidth(), aSource->GetHeight());
		Graphics g(mRankStripImage);
		g.DrawImage(aSource, 0, 0);
		mCopiedImages.push_back(mRankStripImage);
		delete mImages[IMAGE_RANKSTRIP];
		mImages[IMAGE_RANKSTRIP] = NULL;
	}
}

// 0x412000 slot 18: the framework's values, then the current player's profile (high scores are written
// when they change)
void BookWormApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();

	if (mUserManager != NULL)
	{
		User* aUser = mUserManager->FindUser(mUserManager->GetCurrentUserName());
		if (aUser != NULL)
			mUserManager->SaveUser(aUser);
	}
}

// 0x41a540 slot 19 (called by SexyApp::Init)
void BookWormApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	mUserManager = new UserManager(this);
	LoadHighScores();
}

// 0x410970 slot 20: the plain dialog (green Verdana10 look; DoDialog restyles it)
Dialog* BookWormApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog2004(mImages[IMAGE_DIALOG], mImages[IMAGE_DIALOGBUTTON], theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mFonts[FONT_VERDANA10BOLD]);
	aDialog->SetHeaderFont(mFonts[FONT_VERDANA10BOLD]);
	aDialog->SetLinesFont(mFonts[FONT_VERDANA10BOLD]);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 255, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 255, 0));
	aDialog->mContentInsets = Insets(36, 36, 36, 36);
	aDialog->Resize(206, 42, 348, aDialog->GetPreferredHeight(348));
	return aDialog;
}

// 0x414fa0 slot 24
void BookWormApp::URLOpenFailed(const std::string& theURL)
{
	SexyApp::URLOpenFailed(theURL);
	KillDialog(DIALOG_OPENING_BROWSER);

	Dialog* aDialog = NewDialog(DIALOG_URL_FAILED, true, GameText(GT_OPEN_BROWSER_HEADER),
		GameText(GT_OPEN_BROWSER_LINES) + theURL, GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
	aDialog->Resize(100, 50, 440, aDialog->GetPreferredHeight(440));
	AddDialog(DIALOG_URL_FAILED, aDialog);
}

// 0x410b10 slot 25
void BookWormApp::URLOpenSucceeded(const std::string& theURL)
{
	SexyApp::URLOpenSucceeded(theURL);
	KillDialog(DIALOG_OPENING_BROWSER);
}

// 0x415100 slot 26: a wait message (GT_OPENING_BROWSER) until the framework reports (the port's framework
// never opens a URL in a test run: it reports success at once)
bool BookWormApp::OpenURL(const std::string& theURL, bool shutdownOnOpen)
{
	Dialog* aDialog = NewDialog(DIALOG_OPENING_BROWSER, true, "", GameText(GT_OPENING_BROWSER), "",
		Dialog::BUTTONS_NONE);
	aDialog->mContentInsets = Insets(36, 36, 36, 36);
	aDialog->Resize(164, 140, 312, 98);
	AddDialog(DIALOG_OPENING_BROWSER, aDialog);
	return SexyApp::OpenURL(theURL, shutdownOnOpen);
}

// 0x410ca0 slot 33: saves a game in progress (ShutdownWidgets), then the trial counters: with MaxTime
// (60 by default, so always in the retail build) the minutes played are added to the framework's
// TimesPlayed, else with MaxPlays the levels reached
void BookWormApp::Shutdown()
{
	if (mShutdown)
		return;

	ShutdownWidgets();
	if (mMaxTime > 0)
		mTimesPlayed = (int) ((unsigned int) mTimesPlayed + (unsigned int) mPlayTicks / 6000);
	else if (mMaxPlays > 0)
		mTimesPlayed += mPlaysToCount;
	SexyApp::Shutdown();
}

// 0x410110 slot 47: the base already sets the interface's volume; the original repeats it
void BookWormApp::SetMusicVolume(double theVolume)
{
	SexyApp::SetMusicVolume(theVolume);
	if (mMusicInterface != NULL)
	{
		if (mMuteCount > 0)
			mMusicInterface->SetVolume(0);
		else
			mMusicInterface->SetVolume(mMusicVolume);
	}
}

// 0x4103e0 slot 55: every game dialog made through DoDialog gets the big header and white lines
Dialog* BookWormApp::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = SexyApp::DoDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter,
		theButtonMode);
	if (aDialog != NULL)
	{
		aDialog->SetHeaderFont(mFonts[FONT_VERDANA18BOLDCOLOR]);
		aDialog->SetLinesFont(mFonts[FONT_VERDANA12BOLD]);
		aDialog->SetButtonFont(mFonts[FONT_VERDANA12BOLD]);
		aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
		aDialog->SetColor(Dialog::COLOR_LINES, Color(255, 255, 255));
		aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
		if (aDialog->mYesButton != NULL)
			aDialog->mYesButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(255, 255, 255));
		if (aDialog->mNoButton != NULL)
			aDialog->mNoButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(255, 255, 255));
		aDialog->Resize(aDialog->mX, aDialog->mY, aDialog->mWidth, aDialog->GetPreferredHeight(aDialog->mWidth));
	}
	return aDialog;
}

// 0x4110b0 slot 58: the board gets the focus back when the last dialog goes
bool BookWormApp::KillDialog(int theDialogId)
{
	if (SexyApp::KillDialog(theDialogId))
	{
		if ((mDialogMap.size() == 0) && (mBoard != NULL))
			mWidgetManager->SetFocus(mBoard);
		return true;
	}
	return false;
}

// 0x410ad0 slot 60: a modal dialog pauses the board (counted)
void BookWormApp::ModalOpen()
{
	if (mBoard != NULL)
		mBoard->Pause(true);
}

// 0x410af0 slot 61
void BookWormApp::ModalClose()
{
	if (mBoard != NULL)
		mBoard->Pause(false);
}

// 0x419000 slot 62 (after the framework unmuted)
void BookWormApp::GotFocus()
{
	SexyApp::GotFocus();
	if (mBoard != NULL)
	{
		mBoard->Pause(false);
		if (mBoard->mPauseCount == 0)
			PauseAllMusic(false);
		mResumeUpdateCnt = mUpdateCount;
	}
}

// 0x419040 slot 63 (after the framework muted)
void BookWormApp::LostFocus()
{
	SexyApp::LostFocus();
	if (mBoard != NULL)
	{
		if (mBoard->mPauseCount == 0)
			PauseAllMusic(true);
		mBoard->Pause(true);
	}
}

// 0x414ef0 slot 67 (TitleScreen::ButtonDepress): the main menu
void BookWormApp::TitleScreenDone()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	SexyString aName = mUserManager->mCurrentUser;
	if (aName.length() != 0)
		LoadUserSettings();
	ShowGameSelector();

	// port(mod): -replay=<file>: watched at once, as if a game were started from the menu (Replay.h)
	if (!mWatchPath.empty())
	{
		std::string aPath = mWatchPath;
		mWatchPath.clear();
		StartWatch(aPath);
	}
}

// port: the 2004 SexyApp creates a BassMusicInterface (BASS 2.0), which sets the module music's global
// volume to the full music volume (BASS 1.x and the port's default: 0.4 of it): the port's music
// interface gets the full range (docs/FRAMEWORK_2004.md section 6, 4.2). The loudness against BASS 2.0's
// rendering is not calibrated.
MusicInterface* BookWormApp::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new MusicInterface;

	SDLMusicInterface* aMusicInterface = new SDLMusicInterface();
	aMusicInterface->mMaxMusicVolume = 100;
	return aMusicInterface;
}

// port (web): the save Shutdown() would write now (ShutdownWidgets: a game in progress to
// players/<name>/<mode>.dat), taken without changing the board (see SexyAppBase::UpdateUserDataBackup)
void BookWormApp::GetShutdownSaves(std::vector<ShutdownSave>* theSaves)
{
	if ((mShutdown) || (mBoard == NULL) || (!mBoard->IsInProgress()) || (mUserManager == NULL) || (mBoard->mPlayer != NULL))
		return;     // port(mod): a watched replay is not the player's game (Replay.h)

	SexyString aFileName = GetSaveGamePath(mUserManager->GetCurrentUserName(), mGameMode);
	if (aFileName.empty())
		return;

	Buffer aBuffer;
	if (!mBoard->SaveGameSnapshot(mGameMode, &aBuffer))
		return;

	ShutdownSave aSave;
	aSave.mFileName = aFileName;
	aSave.mData.assign((const char*) aBuffer.GetDataPtr(), (size_t) aBuffer.GetDataLen());
	theSaves->push_back(aSave);
}

// port (web): the registry a Shutdown() now would leave: its trial arithmetic adds the minutes played to
// TimesPlayed (ShutdownWidgets first counts a level reached) before SexyApp::Shutdown writes the registry
void BookWormApp::WriteShutdownRegistry()
{
	int aTimesPlayed = mTimesPlayed;
	if (!mShutdown)
	{
		int aPlaysToCount = mPlaysToCount;
		if ((mBoard != NULL) && (mLevelReached))
			aPlaysToCount++;
		if (mMaxTime > 0)
			mTimesPlayed = (int) ((unsigned int) mTimesPlayed + (unsigned int) mPlayTicks / 6000);
		else if (mMaxPlays > 0)
			mTimesPlayed += aPlaysToCount;
	}
	WriteToRegistry();
	mTimesPlayed = aTimesPlayed;
}

// 0x40ffa0 (only Shutdown): removes the screens; a game in progress is saved
void BookWormApp::ShutdownWidgets()
{
	KillDialog(DIALOG_HINT);
	EndWatch();     // port(mod): the app's own state back (Replay.h)

	if (mSidePanel != NULL)
	{
		mWidgetManager->RemoveWidget(mSidePanel);    // port(mod)
		SafeDeleteWidget(mSidePanel);
		mSidePanel = NULL;
	}

	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		SafeDeleteWidget(mTitleScreen);
		mTitleScreen = NULL;
	}

	if (mGameSelector != NULL)
	{
		mWidgetManager->RemoveWidget(mGameSelector);
		SafeDeleteWidget(mGameSelector);
		mGameSelector = NULL;
	}

	if (mFameWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mFameWidget);
		SafeDeleteWidget(mFameWidget);
		mFameWidget = NULL;
	}

	if (mBoard != NULL)
	{
		if (mLevelReached)
		{
			mPlaysToCount++;
			mLevelReached = false;
		}
		if (mBoard->IsInProgress())
			mBoard->SaveGame(mGameMode);
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
}

// 0x410180
void BookWormApp::RemoveBoard()
{
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
}

// 0x4101c0: closes the options, then the help screen covers the (paused, hidden) board and summary
void BookWormApp::ShowHelp()
{
	OptionsDialog* anOptions = (OptionsDialog*) GetDialog(DIALOG_OPTIONS);
	if (anOptions != NULL)
		anOptions->ApplyAndClose();

	if (mHelpWidget == NULL)
	{
		mHelpWidget = new HelpWidget(this);
		mHelpWidget->Resize(GAME_SCREEN_WIDTH, mHeight, GAME_SCREEN_WIDTH, mHeight);
		mWidgetManager->AddWidget(mHelpWidget);
		mHelpWidget->Resize(0, 0, GAME_SCREEN_WIDTH, mHeight);
		if (mBoard != NULL)
		{
			mBoard->Pause(true);
			mBoard->SetVisible(false);
			if (mBoard->mLevelUpWidget != NULL)
			{
				mBoard->mLevelUpWidget->SetDisabled(true);
				mBoard->mLevelUpWidget->SetVisible(false);
			}
		}
	}
}

// 0x4102d0
void BookWormApp::HideHelp()
{
	if (mHelpWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHelpWidget);
		SafeDeleteWidget(mHelpWidget);
		mHelpWidget = NULL;
		if (mBoard != NULL)
		{
			mBoard->Pause(false);
			mBoard->SetVisible(true);
			if ((mBoard->mLevelUpWidget != NULL) && (mBoard->mSummaryShown))
			{
				mBoard->mLevelUpWidget->SetDisabled(false);
				mBoard->mLevelUpWidget->SetVisible(true);
			}
		}
	}
}

// 0x410360
void BookWormApp::RemoveGameSelector()
{
	if (mGameSelector != NULL)
	{
		mWidgetManager->RemoveWidget(mGameSelector);
		SafeDeleteWidget(mGameSelector);
		mGameSelector = NULL;
	}
}

// 0x4103a0
void BookWormApp::RemoveFameWidget()
{
	if (mFameWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mFameWidget);
		SafeDeleteWidget(mFameWidget);
		mFameWidget = NULL;
	}
}

// 0x410d10: a new Board for mGameMode, behind the selector (which slides away)
void BookWormApp::StartGame()
{
	KillDialog(DIALOG_UNUSED_3);
	HideHelp();

	for (int i = 0; i < NUM_SONGS; i++)
	{
		if ((i != SONG_CLASSIC) && (i != SONG_ACTION))
			mMusicInterface->FadeOut(i, true, 0.005);
	}
	// Song 27 does not exist: a no-op (the original's; probably meant for song 0)
	mMusicInterface->FadeOut(27, true, 0.0075);

	// A FadeIn guarded by IsPlaying is skipped while the same song still fades out (it counts as playing)
	if (mGameMode == GAMEMODE_ACTION)
	{
		if (!mMusicInterface->IsPlaying(SONG_ACTION))
			mMusicInterface->FadeIn(SONG_ACTION, ORDER_ACTION, 0.005, false);
	}
	else
		mMusicInterface->FadeOut(SONG_ACTION, true, 0.005);

	if (mGameMode == GAMEMODE_CLASSIC)
	{
		if (!mMusicInterface->IsPlaying(SONG_CLASSIC))
			mMusicInterface->FadeIn(SONG_CLASSIC, ORDER_CLASSIC, 0.005, false);
	}
	else
		mMusicInterface->FadeOut(SONG_CLASSIC, true, 0.005);

	RemoveBoard();
	mBoard = new Board(this);
	mBoard->Resize(0, 0, GAME_SCREEN_WIDTH, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mBoard->ShowOverlay(false);
	mBoard->PickBonusWord();
	if (GetDialog(DIALOG_CONTINUE) != NULL)
		mBoard->mState = BOARD_INACTIVE; // dead in practice: dialog 5 is killed before
	if (mGameSelector != NULL)
		mGameSelector->StartCloseTransition();
	mLevelReached = false;
}

// port(mod): replays (Replay.h): watches the replay thePath, a new board in its mode with the one-time hints as
// they were; EndWatch gives back everything it changes in the app
bool BookWormApp::StartWatch(const std::string& thePath)
{
	Replay::File aFile;
	std::string anError;
	bool isOk = Replay::LoadFile(thePath, &aFile, &anError);
	if ((isOk) && (aFile.mScoreKey != Platform::ModManager::Get().GetScoreKey()))
	{
		isOk = false;
		anError = "it was recorded with other gameplay mods";
	}
	if (!isOk)
	{
		SDL_Log("replay: cannot watch %s: %s", thePath.c_str(), anError.c_str());
		Dialog* aDialog = DoDialog(DIALOG_HINT, true, _S("Replay"), _S("This replay cannot be played: ") + anError + ".",
			GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
		aDialog->Move(((GAME_SCREEN_WIDTH - aDialog->mWidth) + 233) / 2, (mHeight - aDialog->mHeight) / 2);
		return false;
	}
	SDL_Log("replay: watching %s", thePath.c_str());

	if (mWatchFile == NULL)
	{
		mWatchSavedMode = mGameMode;
		mWatchSavedFlags[0] = mWatchOutHintShown;
		mWatchSavedFlags[1] = mFireAlertHintShown;
		mWatchSavedFlags[2] = mGreenTileHintShown;
		mWatchSavedFlags[3] = mScrambleHintShown;
		mWatchSavedFlags[4] = mLevelReached;
		mWatchSavedBonusWord = mBonusWord;
		mWatchSavedRand = *gGameRand;
		Board::GetGlobalState(&mWatchSavedLetterIdx, &mWatchSavedRefillLevel);
	}
	delete mWatchFile;
	mWatchFile = new Replay::File(aFile);
	mGameMode = aFile.mMode;
	mWatchOutHintShown = (aFile.mAppFlags & Replay::APP_WATCH_OUT_HINT) != 0;
	mFireAlertHintShown = (aFile.mAppFlags & Replay::APP_FIRE_ALERT_HINT) != 0;
	mGreenTileHintShown = (aFile.mAppFlags & Replay::APP_GREEN_TILE_HINT) != 0;
	mScrambleHintShown = (aFile.mAppFlags & Replay::APP_SCRAMBLE_HINT) != 0;
	StartGame();    // the board's constructor takes mWatchFile
	return true;
}

void BookWormApp::EndWatch()
{
	if (mWatchFile == NULL)
		return;
	delete mWatchFile;
	mWatchFile = NULL;
	mWatchSpeed = 1;
	mGameMode = mWatchSavedMode;
	mWatchOutHintShown = mWatchSavedFlags[0];
	mFireAlertHintShown = mWatchSavedFlags[1];
	mGreenTileHintShown = mWatchSavedFlags[2];
	mScrambleHintShown = mWatchSavedFlags[3];
	mLevelReached = mWatchSavedFlags[4];
	mBonusWord = mWatchSavedBonusWord;
	*gGameRand = mWatchSavedRand;
	Board::SetGlobalState(mWatchSavedLetterIdx, mWatchSavedRefillLevel);
}

// The watch is over (its game-over page's Continue, MENU or Escape): the main menu slides in over the watched
// board, which the next game removes
void BookWormApp::StopWatch()
{
	EndWatch();
	ShowGameSelector();
}

void BookWormApp::WatchLastReplay()
{
	if ((mBoard != NULL) && (mBoard->mState == BOARD_GAMEOVER) && (mBoard->mPlayer == NULL))
		mBoard->DeleteSavedGame(mGameMode);     // as the game-over page's Continue does
	StartWatch(mLastReplayPath);
}

// 0x410f00: the main menu; a board in play is suspended under it (not deleted) and the selector slides
// in over a snapshot of it
void BookWormApp::ShowGameSelector()
{
	KillDialog(DIALOG_UNUSED_3);
	for (int i = 0; i < NUM_SONGS; i++)
	{
		if (i != SONG_MENU)
			mMusicInterface->FadeOut(i, true, 0.01);
	}

	RemoveGameSelector();
	mGameSelector = new BWGameSelector(this);
	mGameSelector->Resize(0, 0, GAME_SCREEN_WIDTH, mHeight);
	mWidgetManager->AddWidget(mGameSelector);
	if (mBoard != NULL)
	{
		mBoard->Abandon();
		mWidgetManager->RemoveWidget(mBoard);
		mGameSelector->StartOpenTransition();
	}

	if (!mMusicInterface->IsPlaying(SONG_MENU))
		mMusicInterface->FadeIn(SONG_MENU, ORDER_MENU, 0.005, false);
}

// 0x411020
void BookWormApp::ShowHallOfFame()
{
	RemoveFameWidget();
	mFameWidget = new FameWidget(this);
	mFameWidget->Resize(0, 0, GAME_SCREEN_WIDTH, mHeight);
	mWidgetManager->AddWidget(mFameWidget);
}

// 0x413c80 (the selector's StartGame)
void BookWormApp::StartGameForUser()
{
	LoadUserSettings();
	StartGame();
	if (mShowHelp)
		ShowHelp();
}

// 0x4153c0 (the selector's Classic / Action buttons): offers the player's saved game of that mode, else
// starts a game
void BookWormApp::SelectGameMode(int theMode)
{
	mSelectedGameMode = theMode;
	if (HasSavedGame(mUserManager->GetCurrentUserName(), mSelectedGameMode))
	{
		Dialog* aDialog = DoDialog(DIALOG_CONTINUE, true, GameText(GT_CONTINUE_HEADER), GameText(GT_CONTINUE_LINES),
			"", Dialog::BUTTONS_YES_NO);
		aDialog->SetLinesFont(mFonts[FONT_VERDANA10BOLD]);
		aDialog->mTextAlign = -1;
		aDialog->mContentInsets = Insets(36, 32, 36, 36);
		aDialog->mYesButton->mLabel = GameText(GT_CONTINUE_YES);
		aDialog->mNoButton->mLabel = GameText(GT_CONTINUE_NO);
		int aHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
		int aWidth = aDialog->mWidth;
		aDialog->Resize(((GAME_SCREEN_WIDTH - aWidth) + 233) / 2, (mHeight - aHeight) / 2, aWidth,
			aDialog->GetPreferredHeight(aWidth));
		return;
	}

	if (mGameSelector != NULL)
		mGameSelector->StartGame();
}

// 0x4157e0 (the Board's menu button)
void BookWormApp::GoToMainMenu()
{
	if (mBoard == NULL)
	{
		// Unreachable from the Board's button; kept
		if ((mGameSelector != NULL) && (!mGameSelector->mClosing))
		{
			mGameSelector->StartGame();
			return;
		}
	}
	else if (mBoard->mState != BOARD_GAMEOVER)
	{
		Dialog* aDialog = DoDialog(DIALOG_LEAVE_GAME, true, GameText(GT_LEAVE_GAME_HEADER), GameText(GT_LEAVE_GAME_LINES),
			"", Dialog::BUTTONS_YES_NO);
		aDialog->mTextAlign = 0;
		aDialog->mYesButton->mLabel = GameText(GT_LEAVE_GAME_YES);
		aDialog->mNoButton->mLabel = GameText(GT_LEAVE_GAME_NO);
		int aHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
		int aWidth = aDialog->mWidth;
		aDialog->Resize(((GAME_SCREEN_WIDTH - aWidth) + 233) / 2, (mHeight - aHeight) / 2, aWidth,
			aDialog->GetPreferredHeight(aWidth));
		return;
	}

	ShowGameSelector();
}

// 0x410520
void BookWormApp::ShowSelectUserDialog()
{
	SelectUserDialog* aDialog = new SelectUserDialog(this);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->Resize(0, 17, 400, aDialog->GetPreferredHeight(400));
	AddDialog(DIALOG_SELECT_USER, aDialog);
}

// 0x4105d0 (Board::Update): one hint at a time, only with hints on
void BookWormApp::ShowHint(int theHintId, const SexyString& theHeader, const SexyString& theLines,
	const SexyString& theFooter, int theButtonMode)
{
	if ((!HintDialog::WasHintShown(theHintId)) && (mShowHints) && ((mBoard == NULL) || (mBoard->mPlayer == NULL)))
	{
		KillDialog(DIALOG_HINT);
		HintDialog* aDialog = new HintDialog(this, theHintId, theHeader, theLines, theFooter, theButtonMode);
		aDialog->Resize(0, 0, GAME_SCREEN_WIDTH, mHeight);
		aDialog->Resize(233, 110, 400, aDialog->GetPreferredHeight(400));
		AddDialog(DIALOG_HINT, aDialog);
	}
}

// 0x4106c0 (Options): centred over the board area (x offset 233)
void BookWormApp::ShowCredits()
{
	CreditsDialog* aDialog = new CreditsDialog(this);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	aDialog->Resize(0, 47, 388, aDialog->GetPreferredHeight(388));
	aDialog->Move(((GAME_SCREEN_WIDTH - aDialog->mWidth) + 233) / 2, aDialog->mY);
	AddDialog(DIALOG_CREDITS, aDialog);
}

// 0x4107c0 (the Board's options button): starts off-screen right and slides in
void BookWormApp::ShowOptions()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	int aHeight = aDialog->GetPreferredHeight(388);
	aDialog->Resize(mWidth, (mHeight - aHeight) / 2, 388, aHeight);   // port(mod): off the screen, also a wide one
	aDialog->mSlideOffset = 240;
	AddDialog(DIALOG_OPTIONS, aDialog);
}

// 0x4108c0
void BookWormApp::ShowNewUserDialog(const SexyString& theText)
{
	if (mGameSelector != NULL)
		mGameSelector->SetDisabled(true);
	NewUserDialog* aDialog = new NewUserDialog(this, theText);
#ifdef __EMSCRIPTEN__
	SiteFillNewUser(aDialog->mNameEdit);	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with it
#endif
	aDialog->Resize(238, 174, 400, aDialog->GetPreferredHeight(400));
	AddDialog(DIALOG_NEW_USER, aDialog);
}

// 0x415660 (the selector's quit button)
void BookWormApp::ShowQuitDialog()
{
	SexyString aLines = GameText(GT_QUIT_LINES);
	if ((mBoard != NULL) && (mBoard->IsInProgress()))
		aLines += GameText(GT_QUIT_IN_PROGRESS);
	Dialog* aDialog = DoDialog(DIALOG_QUIT, true, GameText(GT_QUIT_HEADER), aLines, "", Dialog::BUTTONS_YES_NO);
	aDialog->Move(290, 138);
}

// 0x418590 (SelectUserDialog)
void BookWormApp::ShowDeleteUserDialog(std::vector<SexyString> theNames)
{
	DeleteUserDialog* aDialog = new DeleteUserDialog(this, theNames);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->Resize(0, 17, 400, aDialog->GetPreferredHeight(400));
	AddDialog(DIALOG_DELETE_USER, aDialog);
}

// 0x413cb0 not ported: the RegisterDialog (registration; the registered build never reaches it: dialogs 4
// and 17 are not created, OptionsDialog's register button is not shown)
void BookWormApp::ShowRegisterDialog()
{
}

// 0x413f30 ShowInvalidCodeDialog not ported (registration: dialog 9)
// 0x414080 ShowAliasDialog not ported (internet high-score alias: a GetStringDialog, dialog 23)
// 0x415a20 ConfirmInternetConnect not ported (internet high scores: dialog 6)
// 0x415c40 AskForUpdateCheck, 0x415de0 DoUpdateCheck not ported (update check: dialogs 7 and 8)
// 0x416090 GetGameSEHInfo not ported (crash report: the base text + mSEHInfo)

// 0x4152a0: `!GetBoolean("NoInternetScores", true) && (!GetBoolean("NoIHSUpsell", true) || mIsRegistered)`,
// false in the retail build. port: always false (internet high scores are not ported)
bool BookWormApp::IsInternetScoresEnabled()
{
	return false;
}

// 0x414910: the original tests that it may write HKEY_LOCAL_MACHINE (the emulated registry always can)
bool BookWormApp::CheckRegistryAccess()
{
	if (!RegistryWriteInteger("test", 1))
	{
		if (!mIsWindowed)
			SwitchScreenMode(true, Is3DAccelerated());
		MsgBox(GameText(GT_ADMIN_REQUIRED), GameText(GT_ADMIN_REQUIRED_HEADER), MB_ICONERROR);
		return false;
	}
	return true;
}

// 0x4144b0
bool BookWormApp::LoadImage(int theImageId)
{
	static SexyString sImagePath = GetString("ImagePath", "images\\");

	SexyString aPath = sImagePath + gImageNames[theImageId];
	mImages[theImageId] = GetImage(aPath);
	if (mImages[theImageId] == NULL)
	{
		Popup("Failed to load " + aPath);
		mLoadingFailed = true;
		return false;
	}
	return true;
}

// 0x4146d0: the font is stored even when it failed
bool BookWormApp::LoadFont(int theFontId)
{
	static SexyString sFontPath = GetString("FontPath", "fonts\\");

	SexyString aPath = sFontPath + gFontNames[theFontId];
	ImageFont* aFont = new ImageFont(this, aPath);
	mFonts[theFontId] = aFont;
	if (!aFont->mFontData->mInitialized)
	{
		Popup("Failed to load " + aPath);
		mLoadingFailed = true;
		return false;
	}
	return true;
}

// 0x41a5b0: one word per line, upper-cased, words shorter than 3 letters skipped. wordlist.txt is
// prefix-compressed: a number on a line is how many leading letters of the previous word the next word
// keeps, and it stays in force for the following lines until the next number ("aa", "2h", "3ed", "ing",
// "s" -> AA (skipped), AAH, AAHED, AAHING, AAHS). Text mode (CR LF reads as LF).
bool BookWormApp::LoadWordList(const SexyString& theFileName, std::set<SexyString>* theSet,
	std::vector<SexyString>* theVector)
{
	enum { BUFFER_SIZE = 256 };
	char aWord[BUFFER_SIZE];          // the word being read
	char aPrevWord[BUFFER_SIZE];      // the previous word (its bytes beyond its length are older words')
	memset(aWord, 0, sizeof(aWord));
	memset(aPrevWord, 0, sizeof(aPrevWord));
	int aPrefixLength = 0;            // the last number read
	int aLength = 0;
	bool inNumber = false;

	// port: a missing file ends the original with an access violation in getc (caught: "Exception raised: "
	// on the console and exit(1)); here it is a load failure
	PFILE* aFile = p_fopen(theFileName.c_str(), "r");
	if (aFile == NULL)
		return false;

	for (;;)
	{
		int aChar = p_fgetc(aFile);
		if (aChar == EOF)
			break;
		if (mShutdown)
		{
			p_fclose(aFile);
			return false;
		}

		char c = (char) aChar;
		if ((c >= '0') && (c <= '9'))
		{
			if (!inNumber)
			{
				aPrefixLength = 0;
				inNumber = true;
			}
			aPrefixLength = aPrefixLength * 10 + (c - '0');
		}
		else if ((c == '\r') || (c == '\n'))
		{
			if (aLength > 0)
			{
				SexyString aString(aWord, aLength);
				memcpy(aPrevWord, aWord, aLength);
				if (aString.length() > 2)
				{
					UpperCaseString(aString);
					if (theSet != NULL)
						theSet->insert(aString);
					if (theVector != NULL)
						theVector->push_back(aString);
				}
				aLength = 0;
			}
			inNumber = false;
		}
		else
		{
			if ((aPrefixLength > 0) && (aLength == 0))
			{
				// port: the original has no bounds check on its 256-byte buffers
				int aCopyLength = std::min(aPrefixLength, (int) BUFFER_SIZE - 1);
				memcpy(aWord, aPrevWord, aCopyLength);
				aLength = aCopyLength;
			}
			if (aLength < BUFFER_SIZE)
				aWord[aLength++] = c;
		}
	}

	p_fclose(aFile);
	// (the original logs "Time to load word list (%s): %d" with OutputDebugString)
	return true;
}

// 0x41a860
bool BookWormApp::LoadDictionary(const SexyString& theFileName, std::set<SexyString>* theSet)
{
	return LoadWordList(theFileName, theSet, NULL);
}

// 0x41a880
bool BookWormApp::LoadBonusWords(const SexyString& theFileName, std::vector<SexyString>* theVector)
{
	return LoadWordList(theFileName, NULL, theVector);
}

// 0x413a40: the current player's options and play count, each only if the profile has it
void BookWormApp::LoadUserSettings()
{
	SexyString aName = mUserManager->GetCurrentUserName();
	if (aName.length() == 0)
		return;
	User* aUser = mUserManager->FindUser(aName);
	if (aUser == NULL)
		return;

	int aValue = 1;
	if (mUserManager->ReadUserInt(aUser->mName, "ShowHelp", &aValue))
		mShowHelp = aValue != 0;
	if (mUserManager->ReadUserInt(aUser->mName, "ShowHints", &aValue))
		mShowHints = aValue != 0;
	if (mUserManager->ReadUserInt(aUser->mName, "ShowDefs", &aValue))
		mShowDefs = aValue != 0;
	if (mUserManager->ReadUserInt(aUser->mName, "TimesPlayed", &aValue))
		mUserTimesPlayed = aValue;
}

// 0x412100 (OptionsDialog::ApplyAndClose)
bool BookWormApp::SaveCurrentUser()
{
	if (mUserManager != NULL)
	{
		SexyString aName = mUserManager->GetCurrentUserName();
		if (aName.length() != 0)
		{
			User* aUser = mUserManager->FindUser(aName);
			if (aUser != NULL)
			{
				mUserManager->SaveUser(aUser);
				return true;
			}
		}
	}
	return false;
}

// 0x412190: only tests that the file opens
bool BookWormApp::HasSavedGame(const SexyString& theUserName, int theMode)
{
	SexyString aPath = GetSaveGamePath(theUserName, theMode);
	if (aPath.empty())
		return false;
	return FileExists(aPath);
}

// port: the path HasSavedGame (0x412190) and Board's SaveGame / LoadGame / DeleteSavedGame build:
// "players" + "\" + name, + "\" only for a name, + "classic.dat" / "action.dat"; other modes have no file.
// The original's is relative to the exe folder; the framework's file helpers put it in the user dir.
// port(mods): a set of mods that changes play saves apart, in "classic-<score key>.dat"
SexyString BookWormApp::GetSaveGamePath(const SexyString& theUserName, int theMode)
{
	const char* aFileName;
	if (theMode == GAMEMODE_CLASSIC)
		aFileName = "classic";
	else if (theMode == GAMEMODE_ACTION)
		aFileName = "action";
	else
		return "";

	SexyString aPath = SexyString(gPlayersDir) + "/" + theUserName;
	if (theUserName.length() != 0)
		aPath += "/";
	return aPath + aFileName + ScoreKeySuffix() + ".dat";
}

// The strings of a REG_MULTI_SZ value up to the first empty one. port: the emulated registry has no
// REG_MULTI_SZ type: the tables are stored as binary data with the same bytes (RegistryWriteData).
static bool ReadMultiString(SexyAppBase* theApp, const std::string& theValueName, std::vector<SexyString>* theStrings)
{
	theStrings->clear();

	ulong aLength = 0;
	theApp->RegistryReadData(theValueName, NULL, &aLength); // the size, like RegQueryValueEx without a buffer
	if (aLength == 0)
		return false;

	std::vector<uchar> aData(aLength + 1, 0);
	if (!theApp->RegistryReadData(theValueName, &aData[0], &aLength))
		return false;
	aData[aLength] = 0;

	const char* aPtr = (const char*) &aData[0];
	const char* anEnd = aPtr + aLength;
	while ((aPtr < anEnd) && (*aPtr != 0))
	{
		SexyString aString(aPtr);
		theStrings->push_back(aString);
		aPtr += aString.length() + 1;
	}
	return true;
}

// Writes a REG_MULTI_SZ value: every string NUL-terminated, then one more NUL; nothing for no strings
// port(mods): the registry value of high-score table theTable ("Classic"): under the key Scores, or with a set
// of mods that changes play under "Scores-<score key>", so those games have tables of their own
static std::string ScoresValueName(const char* theTable)
{
	return "Scores" + ScoreKeySuffix() + "\\" + theTable;
}

static void WriteMultiString(SexyAppBase* theApp, const std::string& theValueName, const std::vector<SexyString>& theStrings)
{
	std::string aData;
	for (size_t i = 0; i < theStrings.size(); i++)
	{
		aData += theStrings[i];
		aData += '\0';
	}
	if (aData.length() == 0)
		return;
	aData += '\0';
	theApp->RegistryWriteData(theValueName, (const uchar*) aData.data(), (ulong) aData.length());
}

// 0x419080 (ReadFromRegistry): the four tables of the registry key Scores, then the defaults where a table
// is short. Classic / Action rows are name, score; Best rows name, word, score; Longest rows name, word
// (the score is the word's length). No size limit on load.
void BookWormApp::LoadHighScores()
{
	// port: the original's loops step twice per row and test for the end only after the second step, so
	// a value with an odd string count (not a multiple of 3 for Best) loops for ever; these stop at the end
	std::vector<SexyString> aStrings;

	if (ReadMultiString(this, ScoresValueName("Classic"), &aStrings))
	{
		for (size_t i = 0; i < aStrings.size(); )
		{
			HighScoreEntry anEntry;
			anEntry.mName = aStrings[i++];
			anEntry.mScore = 0;
			anEntry.mIsNew = false;
			if (i < aStrings.size())
				anEntry.mScore = atoi(aStrings[i++].c_str());
			mClassicScores.insert(anEntry);
		}
	}

	if (ReadMultiString(this, ScoresValueName("Action"), &aStrings))
	{
		for (size_t i = 0; i < aStrings.size(); )
		{
			HighScoreEntry anEntry;
			anEntry.mName = aStrings[i++];
			anEntry.mScore = 0;
			anEntry.mIsNew = false;
			if (i < aStrings.size())
				anEntry.mScore = atoi(aStrings[i++].c_str());
			mActionScores.insert(anEntry);
		}
	}

	if (ReadMultiString(this, ScoresValueName("Best"), &aStrings))
	{
		for (size_t i = 0; i < aStrings.size(); )
		{
			HighScoreEntry anEntry;
			anEntry.mName = aStrings[i++];
			anEntry.mScore = 0;
			anEntry.mIsNew = false;
			if (i < aStrings.size())
				anEntry.mWord = aStrings[i++];
			if (i < aStrings.size())
				anEntry.mScore = atoi(aStrings[i++].c_str());
			mBestWords.insert(anEntry);
		}
	}

	if (ReadMultiString(this, ScoresValueName("Longest"), &aStrings))
	{
		for (size_t i = 0; i < aStrings.size(); )
		{
			HighScoreEntry anEntry;
			anEntry.mName = aStrings[i++];
			anEntry.mIsNew = false;
			if (i < aStrings.size())
				anEntry.mWord = aStrings[i++];
			anEntry.mScore = (int) anEntry.mWord.length();
			mLongestWords.insert(anEntry);
		}
	}

	AddDefaultHighScores();
}

// 0x418bd0: a default row goes in only while its table is short and has no equivalent row (ByScore: same
// score and word; ByWordLength: same word length and score)
void BookWormApp::AddDefaultHighScores()
{
	for (int i = 0; i < NUM_CLASSIC_SCORES; i++)
	{
		HighScoreEntry anEntry;
		anEntry.mName = GameText(gDefaultClassicScores[i].mName);
		anEntry.mWord = GameText(gDefaultClassicScores[i].mWord);
		anEntry.mScore = gDefaultClassicScores[i].mScore;
		anEntry.mIsNew = false;
		if ((mClassicScores.size() < NUM_CLASSIC_SCORES) && (mClassicScores.find(anEntry) == mClassicScores.end()))
			mClassicScores.insert(anEntry);
	}

	for (int i = 0; i < NUM_ACTION_SCORES; i++)
	{
		HighScoreEntry anEntry;
		anEntry.mName = GameText(gDefaultActionScores[i].mName);
		anEntry.mWord = GameText(gDefaultActionScores[i].mWord);
		anEntry.mScore = gDefaultActionScores[i].mScore;
		anEntry.mIsNew = false;
		if ((mActionScores.size() < NUM_ACTION_SCORES) && (mActionScores.find(anEntry) == mActionScores.end()))
			mActionScores.insert(anEntry);
	}

	for (int i = 0; i < NUM_WORD_SCORES; i++)
	{
		HighScoreEntry anEntry;
		anEntry.mName = GameText(gDefaultLongestWords[i].mName);
		anEntry.mWord = GameText(gDefaultLongestWords[i].mWord);
		anEntry.mScore = gDefaultLongestWords[i].mScore;
		anEntry.mIsNew = false;
		if ((mLongestWords.size() < NUM_WORD_SCORES) && (mLongestWords.find(anEntry) == mLongestWords.end()))
			mLongestWords.insert(anEntry);
	}

	for (int i = 0; i < NUM_WORD_SCORES; i++)
	{
		HighScoreEntry anEntry;
		anEntry.mName = GameText(gDefaultBestWords[i].mName);
		anEntry.mWord = GameText(gDefaultBestWords[i].mWord);
		anEntry.mScore = gDefaultBestWords[i].mScore;
		anEntry.mIsNew = false;
		if ((mBestWords.size() < NUM_WORD_SCORES) && (mBestWords.find(anEntry) == mBestWords.end()))
			mBestWords.insert(anEntry);
	}
}

// 0x412ec0: Classic, Action and Best with their scores as text, Longest without (its score is the word
// length); an empty table is not written
void BookWormApp::SaveHighScores()
{
	std::vector<SexyString> aStrings;

	aStrings.clear();
	for (HighScoreByScoreSet::const_iterator anItr = mClassicScores.begin(); anItr != mClassicScores.end(); ++anItr)
	{
		aStrings.push_back(anItr->mName);
		aStrings.push_back(StrFormat("%d", anItr->mScore));
	}
	WriteMultiString(this, ScoresValueName("Classic"), aStrings);

	aStrings.clear();
	for (HighScoreByScoreSet::const_iterator anItr = mActionScores.begin(); anItr != mActionScores.end(); ++anItr)
	{
		aStrings.push_back(anItr->mName);
		aStrings.push_back(StrFormat("%d", anItr->mScore));
	}
	WriteMultiString(this, ScoresValueName("Action"), aStrings);

	aStrings.clear();
	for (HighScoreByScoreSet::const_iterator anItr = mBestWords.begin(); anItr != mBestWords.end(); ++anItr)
	{
		aStrings.push_back(anItr->mName);
		aStrings.push_back(anItr->mWord);
		aStrings.push_back(StrFormat("%d", anItr->mScore));
	}
	WriteMultiString(this, ScoresValueName("Best"), aStrings);

	aStrings.clear();
	for (HighScoreByLengthSet::const_iterator anItr = mLongestWords.begin(); anItr != mLongestWords.end(); ++anItr)
	{
		aStrings.push_back(anItr->mName);
		aStrings.push_back(anItr->mWord);
	}
	WriteMultiString(this, ScoresValueName("Longest"), aStrings);
}

// Keeps the first theCount entries of a table (in order)
template <class T> static void PruneTable(T& theTable, int theCount)
{
	int anIndex = 0;
	typename T::iterator anItr = theTable.begin();
	while (anItr != theTable.end())
	{
		if (anIndex < theCount)
			++anItr;
		else
			anItr = theTable.erase(anItr);
		anIndex++;
	}
}

// 0x419cd0 (Board::Update at game over): the game's score, best word and longest word go into their
// tables; Board::mNewHighScore is set when an entry equivalent to the new one is in a table afterwards
// (an equal score with the same word counts, even if the new entry itself was pruned)
void BookWormApp::SubmitHighScores(StatKeeper* theStats)
{
	HighScoreEntry anEntry;
	anEntry.mName = mUserManager->GetCurrentUserName();
	anEntry.mWord = "";
	anEntry.mScore = theStats->mScore;
	anEntry.mIsNew = true;

	HighScoreByScoreSet* aTable = NULL;
	int aTableSize = 0;
	if (mGameMode == GAMEMODE_ACTION)
	{
		aTable = &mActionScores;
		aTableSize = NUM_ACTION_SCORES;
	}
	else if (mGameMode == GAMEMODE_CLASSIC)
	{
		aTable = &mClassicScores;
		aTableSize = NUM_CLASSIC_SCORES;
	}
	if (aTable != NULL)
	{
		aTable->insert(anEntry);
		if ((int) aTable->size() > aTableSize)
			PruneTable(*aTable, aTableSize);
		if (aTable->find(anEntry) != aTable->end())
			mBoard->mNewHighScore = true;
	}

	anEntry.mWord = theStats->mBestWord;
	anEntry.mScore = theStats->mBestWordScore;
	mBestWords.insert(anEntry);
	if (mBestWords.size() > NUM_WORD_SCORES)
	{
		PruneTable(mBestWords, NUM_WORD_SCORES);
		if (mBestWords.find(anEntry) != mBestWords.end())
			mBoard->mNewHighScore = true;
	}

	anEntry.mWord = theStats->mLongestWord;
	anEntry.mScore = theStats->mLongestWordScore;
	mLongestWords.insert(anEntry);
	if (mLongestWords.size() > NUM_WORD_SCORES)
	{
		PruneTable(mLongestWords, NUM_WORD_SCORES);
		if (mLongestWords.find(anEntry) != mLongestWords.end())
			mBoard->mNewHighScore = true;
	}

	SaveHighScores();
}

// 0x4184d0: pause remembers the songs that were playing; resume restarts those (a second pause before a
// resume finds them paused, not playing, and keeps the list)
void BookWormApp::PauseAllMusic(bool pause)
{
	if (pause)
	{
		for (int i = 0; i < NUM_SONGS; i++)
		{
			if (mMusicInterface->IsPlaying(i))
			{
				mPausedSongs.push_back(i);
				mMusicInterface->PauseMusic(i);
			}
		}
		return;
	}

	for (size_t i = 0; i < mPausedSongs.size(); i++)
		mMusicInterface->ResumeMusic(mPausedSongs[i]);
	std::vector<int>().swap(mPausedSongs);
}

// 0x411cd0: "[d:][hh:]mm:ss", "" without a board
SexyString BookWormApp::FormatTime(int theSeconds)
{
	SexyString aResult;
	if (mBoard != NULL)
	{
		int aDays = theSeconds / 86400;
		int anHours = (theSeconds / 3600) % 24;
		int aSecs = theSeconds % 60;
		if (aDays > 0)
			aResult += StrFormat("%d:", aDays);
		if ((anHours > 0) || (aDays > 0))
			aResult += StrFormat("%02d:", anHours);
		aResult += StrFormat("%02d:", (theSeconds / 60) % 60);
		aResult += StrFormat("%02d", aSecs);
	}
	return aResult;
}

// 0x411ea0 (LevelUpWidget, GT_SUMMARY_TIME)
SexyString BookWormApp::GetLevelTimeString(const SexyString& thePrefix)
{
	if (mBoard == NULL)
		return thePrefix;
	return thePrefix + FormatTime(mBoard->mLevelSeconds);
}

// 0x411f50 (LevelUpWidget, GT_SUMMARY_TOTAL)
SexyString BookWormApp::GetTotalTimeString(const SexyString& thePrefix)
{
	if (mBoard == NULL)
		return thePrefix;
	return thePrefix + FormatTime(mBoard->mTotalSeconds);
}
