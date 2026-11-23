// Sexy::ClassicEndLevelDialog (bejeweled3, 0x7842b0 over 0x6a7c80). See EndLevelDialog.h.
#include "EndLevelDialog.h"
#include "IceStormBoard.h"
#include "DiamondMineBoard.h"

#include "Bej3App.h"
#include "Board.h"
#include "Common.h"
#include "CrystalBall.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "PokerBoard.h"
#include "Res.h"
#include "X86Math.h"

#include <cmath>
#include <cstdio>

namespace Sexy {

namespace {

// The exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure)
{
	return gApp->mArtRes * theMeasure / 0x4b0;
}

// 0x475e30: the text of a numbered string in default.xml
std::string TextById(int theId, const std::string& theDefault)
{
	return gApp->GetString(std::to_string(theId), theDefault);
}

// the board's counters the dialog shows (the exe's names for them are not known)
enum { COUNTER_FLAME_GEMS = 17, COUNTER_STAR_GEMS = 18, COUNTER_HYPERCUBES = 19,
	COUNTER_LONGEST_CASCADE = 24, COUNTER_BEST_MOVE = 25 };

const Color TRANSPARENT_COLOR(0, 0, 0, 0);

// the colours of a font's layers for a while (0x58f620 and 0x58f920)
class LayerColors {
public:
	explicit LayerColors(Font* theFont) : mFont(theFont) {}
	~LayerColors()
	{
		for (int i = mCount - 1; i >= 0; i--)
			RestoreLayerColor(mFont, mNames[i], mOld[i]);
	}
	void Set(const char* theName, const Color& theColor)
	{
		if (mCount < 3 && SetLayerColor(mFont, theName, theColor, &mOld[mCount]))
			mNames[mCount++] = theName;
	}

private:
	Font* mFont;
	const char* mNames[3] = {};
	Color mOld[3];
	int mCount = 0;
};

// the picture's place from resources.xml, then the exe's shift, scaled
void DrawImageAt(Graphics* g, Image* theImage, int theImageId, int theShiftX, int theShiftY)
{
	g->DrawImage(theImage, Scaled(GetImageOffsetX(theImageId) + theShiftX), Scaled(GetImageOffsetY(theImageId) + theShiftY));
}

// port: the framework's DialogButton draws the three cels of the picture (rest, over, pressed) from these rectangles
DialogButton* MakeButton(Bej3Dialog* theDialog, Image* theImage, int theId, const std::string& theLabel)
{
	DialogButton* aButton = new DialogButton(theImage, theId, theDialog);
	aButton->SetFont(FONT_DIALOG_BUTTONS);
	aButton->mLabel = theLabel;
	aButton->mNormalRect = theImage->GetCelRect(0);
	aButton->mOverRect = theImage->GetCelRect(1);
	aButton->mDownRect = theImage->GetCelRect(2);
	theDialog->AddWidget(aButton);
	return aButton;
}

}  // namespace

// 0x7842b0 with 0x6a7c80 (bejeweled3): a modal Bej3Dialog with id 0x26 and no text, 1600 by 1200 (1200ths), a bottom inset of 0x3c.
// It takes the board's counters, score, level and the rank of the score just handed to the leaderboard. Buttons (each 0x118 wide,
// the exe lifts them 0x28 for Classic): MAIN MENU (id 0, string 0xe3) right of the middle, BADGES (id 3, string 0xe4) left of it,
// PLAY AGAIN (id 2, string 0xe5) in the middle and wider. The score counts up from 0 on a curve over about 60 updates.
// port: the Quest game's NEW QUEST button (id 1) and the rank bar widget (0x682f90, id 0x4ab, at (200, 270)) are not made; the
// exe's mouse-state sets and the mode's own checks for the Quest are left out
ClassicEndLevelDialog::ClassicEndLevelDialog(Board* theBoard) :
	Bej3Dialog(DIALOG_ID, true, "", "", "", BUTTONS_NONE)
{
	Resize(0, 0, Scaled(1600), Scaled(1200));
	mContentInsets.mBottom = Scaled(0x3c);
	mMouseDownAllowed = false;
	for (int i = 0; i < 38; i++)
		mCounters[i] = theBoard->mCounters[i];
	mScore = theBoard->mScorePoints;
	mLevelIndex = theBoard->mLevelIndex;
	mLevelNumber = theBoard->mLevelNumber;
	mScoreRank = theBoard->mLastScoreRank;
	mScores = theBoard->mHighScores;
	mNames = theBoard->mHighScoreNames;
	mNames.resize(mScores.size());
	mScoreCurve.SetCurve("b+0,1,0.016667,1,####  M#1^;       S~TEC");

	const int aLift = Scaled(0x28);
	const int aSmallWidth = Scaled(0x118);
	const int aSmallHeight = IMAGE_DIALOG_SMALL_BUTTON->GetCelHeight();
	mMainMenuButton = MakeButton(this, IMAGE_DIALOG_SMALL_BUTTON, BUTTON_MAIN_MENU, TextById(0xe3, "MAIN MENU"));
	mMainMenuButton->Resize(Scaled(0x145) + mWidth / 2 - aSmallWidth / 2,
		mHeight - mContentInsets.mBottom - aSmallHeight - aLift, aSmallWidth, aSmallHeight);
	mBadgesButton = MakeButton(this, IMAGE_DIALOG_SMALL_BUTTON, BUTTON_BADGES, TextById(0xe4, "BADGES"));
	mBadgesButton->Resize(mWidth / 2 - Scaled(0x145) - aSmallWidth / 2,
		mHeight - mContentInsets.mBottom - aSmallHeight - aLift, aSmallWidth, aSmallHeight);
	const int aBigWidth = Scaled(0x118 + 0x50);
	const int aBigHeight = IMAGE_DIALOG_BUTTON->GetCelHeight();
	mPlayAgainButton = MakeButton(this, IMAGE_DIALOG_BUTTON, BUTTON_PLAY_AGAIN, TextById(0xe5, "PLAY AGAIN"));
	mPlayAgainButton->Resize((mWidth - aBigWidth) / 2,
		mHeight - mContentInsets.mBottom - aBigHeight + Scaled(10) - aLift, aBigWidth, aBigHeight);
}

ClassicEndLevelDialog::~ClassicEndLevelDialog()
{
	delete mMainMenuButton;
	delete mBadgesButton;
	delete mPlayAgainButton;
}

// 0x784340 (bejeweled3): only while no dialog 0x27 (the badges' dialog) is up: the dialog's own update and the score's count up
void ClassicEndLevelDialog::Update()
{
	if (gApp->GetDialog(0x27) != nullptr)
		return;
	Bej3Dialog::Update();
	mScoreCurve.IncInVal();
}

// 0x6a91e0 (bejeweled3): the release sound and the Dialog's, then MAIN MENU closes the dialog and the game screen leaves for the
// play menu (slot 232), PLAY AGAIN closes it and the board starts a new game (slots 85 and 135).
// port: BADGES opens the badges dialog (id 0x2a), which is not made; the music's FadeOut and Play "Classic" that PLAY AGAIN queues
// are left out (the board's own start queues the mode's music)
void ClassicEndLevelDialog::ButtonDepress(int theId)
{
	if (mClosing)
		return;
	Bej3Dialog::ButtonDepress(theId);
	Board* aBoard = gApp->mBoard;
	if (theId == BUTTON_MAIN_MENU) {
		StartClose();
		if (aBoard != nullptr)
			aBoard->LeaveToMenu();
	}
	else if (theId == BUTTON_PLAY_AGAIN) {
		StartClose();
		if (aBoard != nullptr) {
			aBoard->ResetGame();
			aBoard->NewGame();
		}
	}
}

// 0x6a87a0 (bejeweled3): the dialog's picture, the stamp, "Final Score:" and the score counting up, then the content (0x7839f0)
void ClassicEndLevelDialog::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(Scaled(0x6e), 0, Scaled(0x564), Scaled(0x4b0)), IMAGE_GAMEOVER_DIALOG);
	g->DrawImage(IMAGE_GAMEOVER_STAMP, Scaled(GetImageOffsetX(IMAGE_GAMEOVER_STAMP_ID) - 0xa0),
		Scaled(GetImageOffsetY(IMAGE_GAMEOVER_STAMP_ID)));
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_GAMEOVER_DIALOG_HUGE);
	{
		LayerColors aLayers(FONT_GAMEOVER_DIALOG_HUGE);
		aLayers.Set("MAIN", Color(0x88, 0x48, 0x18, 255));
		aLayers.Set("LAYER_2", Color(0xe8, 0xc0, 0x90, 255));
		aLayers.Set("LAYER_3", TRANSPARENT_COLOR);
		g->WriteString(TextById(0xec, "Final Score:"), Scaled(800), Scaled(0x8c), 0, 0, true, 0, -1, -1);
	}
	{
		LayerColors aLayers(FONT_GAMEOVER_DIALOG_HUGE);
		aLayers.Set("MAIN", Color(255, 255, 255, 255));
		aLayers.Set("LAYER_2", Color(0xb0, 0x60, 0x30, 255));
		aLayers.Set("LAYER_3", TRANSPARENT_COLOR);
		int aShown = Ftol(mScoreCurve.GetOutVal() * (double)mScore);
		g->WriteString(CommaSeperate(aShown), Scaled(800), Scaled(0xdc), 0, 0, true, 0, -1, -1);
	}
	DrawContent(g);
}

// 0x7839f0 (bejeweled3), slot 94: the sections go 0x3c down, the special gems' counts 0x32 up from there
void ClassicEndLevelDialog::DrawContent(Graphics* g)
{
	g->Translate(0, Scaled(0x3c));
	DrawStatisticsPanel(g);
	DrawTopScoresPanel(g);
	g->Translate(0, Scaled(-0x32));
	DrawSpecialGems(g);
}

// 0x6a6470 (bejeweled3), slot 93: the Statistics label, then the table 0x3c lower
void ClassicEndLevelDialog::DrawStatisticsPanel(Graphics* g)
{
	g->DrawImageBox(Rect(Scaled(0xc3), Scaled(0x181), Scaled(0x258), IMAGE_GAMEOVER_SECTION_LABEL->GetHeight()),
		IMAGE_GAMEOVER_SECTION_LABEL);
	g->SetFont(FONT_GAMEOVER_DIALOG);
	{
		LayerColors aLayers(FONT_GAMEOVER_DIALOG);
		aLayers.Set("MAIN", Color(0x88, 0x48, 0x18, 255));
		aLayers.Set("OUTLINE", Color(255, 255, 255, 255));
		aLayers.Set("GLOW", TRANSPARENT_COLOR);
		g->WriteString(TextById(0xeb, "Statistics"), Scaled(0x1e5), Scaled(0x1b3), 0, 0, true, 0, -1, -1);
	}
	g->PushState();
	g->Translate(0, Scaled(0x3c));
	DrawStatisticsTable(g);
	g->PopState();
}

// 0x6a60d0 (bejeweled3), slot 92: the box and the four rows of the table
void ClassicEndLevelDialog::DrawStatisticsTable(Graphics* g)
{
	g->DrawImageBox(Rect(Scaled(0xc3), Scaled(0x181), Scaled(0x25a), Scaled(0x11a)), IMAGE_GAMEOVER_LIGHT_BOX);
	g->SetFont(FONT_GAMEOVER_DIALOG);
	g->SetColor(Color(255, 255, 255, 255));
	LayerColors aLayers(FONT_GAMEOVER_DIALOG);
	aLayers.Set("MAIN", Color(255, 255, 255, 255));
	aLayers.Set("OUTLINE", Color(0x40, 0x40, 0, 255));
	aLayers.Set("GLOW", TRANSPARENT_COLOR);
	DrawStatisticsLabels(g);
	DrawStatisticsValues(g);
	g->SetColor(Color(255, 255, 255, 255));
}

// 0x783a80 (bejeweled3), slot 89: Level Achieved, Best Move, Longest Cascade and Total Time (strings 0xa5 to 0xa8), left
void ClassicEndLevelDialog::DrawStatisticsLabels(Graphics* g)
{
	static const char* const aDefaults[4] = {"Level Achieved:", "Best Move:", "Longest Cascade:", "Total Time:"};
	for (int i = 0; i < 4; i++)
		g->WriteString(TextById(0xa5 + i, aDefaults[i]), Scaled(0xe6), Scaled(0x1db + 0x30 * i), -1, -1, true, 0, -1, -1);
}

// 0x783f00 (bejeweled3), slot 90: the values, right at 0x2f8: the level, the best move and longest cascade (counters 25 and 24)
// and the game's seconds as minutes and seconds (string 0xa9, "%d:%02d").
// port: counters 0, 24 and 25 are written by code of the exe that is not translated (the clock and the combo tracking), so they stay 0
void ClassicEndLevelDialog::DrawStatisticsValues(Graphics* g)
{
	const int aX = Scaled(0x2f8);
	g->WriteString(CommaSeperate(mLevelIndex + 1), aX, Scaled(0x1db), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mCounters[COUNTER_BEST_MOVE]), aX, Scaled(0x1db + 0x30), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mCounters[COUNTER_LONGEST_CASCADE]), aX, Scaled(0x1db + 0x60), -1, 1, true, 0, -1, -1);
	char aTime[32];
	snprintf(aTime, sizeof(aTime), TextById(0xa9, "%d:%02d").c_str(), mCounters[COUNTER_SECONDS] / 60,
		mCounters[COUNTER_SECONDS] % 60);
	g->WriteString(aTime, aX, Scaled(0x1db + 0x90), -1, 1, true, 0, -1, -1);
}

// 0x6a7840 (bejeweled3), slot 91: the Top Scores label, then the list 0x3c lower
void ClassicEndLevelDialog::DrawTopScoresPanel(Graphics* g)
{
	g->DrawImageBox(Rect(Scaled(0x320), Scaled(0x181), Scaled(0x258), IMAGE_GAMEOVER_SECTION_LABEL->GetHeight()),
		IMAGE_GAMEOVER_SECTION_LABEL);
	g->SetFont(FONT_GAMEOVER_DIALOG);
	{
		LayerColors aLayers(FONT_GAMEOVER_DIALOG);
		aLayers.Set("MAIN", Color(0x88, 0x48, 0x18, 255));
		aLayers.Set("OUTLINE", Color(255, 255, 255, 255));
		aLayers.Set("GLOW", TRANSPARENT_COLOR);
		g->WriteString(TextById(0xea, "Top Scores:"), Scaled(0x43d), Scaled(0x1b3), 0, 0, true, 0, -1, -1);
	}
	g->Translate(0, Scaled(0x3c));
	DrawTopScores(g);
}

// 0x6a68b0 (bejeweled3), slot 88: the box and five rows (from y 0x1c7, 0x2d apart): rank, name, score. The row of the score just made
// is white with a pulsing yellow glow, the others gold. port: the exe filters the name to the characters the font has (0x712fa0)
void ClassicEndLevelDialog::DrawTopScores(Graphics* g)
{
	g->DrawImageBox(Rect(Scaled(0x320), Scaled(0x181), Scaled(0x258), Scaled(0x118)), IMAGE_GAMEOVER_LIGHT_BOX);
	g->SetFont(FONT_GAMEOVER_DIALOG);
	int aRow = 0;
	for (int aY = 0x1c7; aY < 0x2a8; aY += 0x2d, aRow++) {
		if (aRow >= (int)mScores.size())
			break;
		const bool aIsNew = aRow == mScoreRank;
		LayerColors aLayers(FONT_GAMEOVER_DIALOG);
		aLayers.Set("OUTLINE", Color(0x40, 0x40, 0, 255));
		if (aIsNew) {
			g->SetColor(Color(255, 255, 255, 255));
			aLayers.Set("GLOW", Color(255, 255, 0, 215 + Ftol(40.0 * sin(0.07 * (double)mUpdateCnt))));
		}
		else {
			g->SetColor(Color(0xff, 0xcb, 0x58, 255));
			aLayers.Set("GLOW", TRANSPARENT_COLOR);
		}
		char aRank[16];
		snprintf(aRank, sizeof(aRank), TextById(0xe9, "%d.").c_str(), aRow + 1);
		g->WriteString(aRank, Scaled(0x33e), Scaled(aY), -1, -1, true, 0, -1, -1);
		std::string aScore = CommaSeperate(mScores[aRow]);
		std::string aName = mNames[aRow];
		const int aNameWidth = Scaled(455) - FONT_GAMEOVER_DIALOG->StringWidth(aScore);   // 0x7131d0: cut to fit
		while (!aName.empty() && FONT_GAMEOVER_DIALOG->StringWidth(aName) > aNameWidth)
			aName.pop_back();
		g->WriteString(aName, Scaled(0x370), Scaled(aY), -1, -1, true, 0, -1, -1);
		g->WriteString(aScore, Scaled(0x55a), Scaled(aY), -1, 1, true, 0, -1, -1);
	}
	g->SetColor(Color(255, 255, 255, 255));
}

// 0x6a70f0 (bejeweled3), slot 87: the pictures of the section below and the three special gems, and "x %d" under each with the
// counters 17 (Flame Gems), 18 (Star Gems) and 19 (Hypercubes) (strings 0xe6 to 0xe8).
// port: the counters are written by code of the exe that is not translated (the special gems' creation counts), so they stay 0
void ClassicEndLevelDialog::DrawSpecialGems(Graphics* g)
{
	DrawImageAt(g, IMAGE_GAMEOVER_SECTION_SMALL, IMAGE_GAMEOVER_SECTION_SMALL_ID, -0xc3, 0x118);
	DrawImageAt(g, IMAGE_GAMEOVER_ICON_FLAME_LRG, IMAGE_GAMEOVER_ICON_FLAME_LRG_ID, -0x82, 0xaf);
	DrawImageAt(g, IMAGE_GAMEOVER_ICON_STAR_LRG, IMAGE_GAMEOVER_ICON_STAR_LRG_ID, 0x3c, 0xaf);
	DrawImageAt(g, IMAGE_GAMEOVER_ICON_HYPERCUBE_LRG, IMAGE_GAMEOVER_ICON_HYPERCUBE_LRG_ID, 0xfa, 0xaa);
	g->SetFont(FONT_GAMEOVER_DIALOG);
	LayerColors aLayers(FONT_GAMEOVER_DIALOG);
	aLayers.Set("GLOW", TRANSPARENT_COLOR);
	aLayers.Set("OUTLINE", TRANSPARENT_COLOR);
	g->SetColor(Color(255, 255, 255, 255));
	static const int aXs[3] = {400, 0x30c, 0x47e};
	static const int aCounters[3] = {COUNTER_FLAME_GEMS, COUNTER_STAR_GEMS, COUNTER_HYPERCUBES};
	for (int i = 0; i < 3; i++) {
		char aText[32];
		snprintf(aText, sizeof(aText), TextById(0xe6 + i, "x %d").c_str(), mCounters[aCounters[i]]);
		g->WriteString(aText, Scaled(aXs[i]), Scaled(900), -1, -1, true, 0, -1, -1);
	}
}

// 0x784c30 (bejeweled3): the dialog of 0x6a7c80 (the leaderboard of the mode's name "Butterflies")
ButterflyEndLevelDialog::ButterflyEndLevelDialog(Board* theBoard) :
	ClassicEndLevelDialog(theBoard)
{
}

// 0x784400 (bejeweled3), slot 89: Butterflies Freed, Best Move, Best Butterfly Combo and Total Time (strings 0x9f to 0xa2), left
void ButterflyEndLevelDialog::DrawStatisticsLabels(Graphics* g)
{
	static const char* const aDefaults[4] = {"Butterflies Freed", "Best Move", "Best Butterfly Combo", "Total Time"};
	for (int i = 0; i < 4; i++)
		g->WriteString(TextById(0x9f + i, aDefaults[i]), Scaled(0xe6), Scaled(0x1db + 0x30 * i), -1, -1, true, 0, -1, -1);
}

// 0x784880 (bejeweled3), slot 90: the values, right at 0x2ee: the butterflies freed (counter 0x1c), the best move (counter 25), the best combo of
// butterflies (counter 0x1d) and the game's seconds (counter 0) as minutes and seconds (string 0xa3, "%d:%02d")
void ButterflyEndLevelDialog::DrawStatisticsValues(Graphics* g)
{
	const int aX = Scaled(0x2ee);
	g->WriteString(CommaSeperate(mCounters[0x1c]), aX, Scaled(0x1db), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mCounters[COUNTER_BEST_MOVE]), aX, Scaled(0x1db + 0x30), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mCounters[0x1d]), aX, Scaled(0x1db + 0x60), -1, 1, true, 0, -1, -1);
	char aTime[32];
	snprintf(aTime, sizeof(aTime), TextById(0xa3, "%d:%02d").c_str(), mCounters[COUNTER_SECONDS] / 60, mCounters[COUNTER_SECONDS] % 60);
	g->WriteString(aTime, aX, Scaled(0x1db + 0x90), -1, 1, true, 0, -1, -1);
}

// 0x6d3180 (bejeweled3): the dialog of 0x6a7c80 (the leaderboard of the mode's name "Icestorm") with the board's best chain (+0x3670)
IceStormEndLevelDialog::IceStormEndLevelDialog(IceStormBoard* theBoard) :
	ClassicEndLevelDialog(theBoard),
	mBestChain(theBoard->mBestChain)
{
}

// 0x6a5f50 (bejeweled3), slot 94: the statistics table and the top scores, as Lightning's
void IceStormEndLevelDialog::DrawContent(Graphics* g)
{
	DrawStatisticsTable(g);
	DrawTopScores(g);
}

// 0x78fbb0 (bejeweled3), slot 89: Highest Multiplier, Columns Crushed, Best Column Combo and Total Time (strings 0x10f, 0x111, 0x110 and 0x112), left
void IceStormEndLevelDialog::DrawStatisticsLabels(Graphics* g)
{
	static const int aStrings[4] = {0x10f, 0x111, 0x110, 0x112};
	static const char* const aDefaults[4] = {"Highest Multiplier", "Columns Crushed", "Best Column Combo", "Total Time"};
	for (int i = 0; i < 4; i++)
		g->WriteString(TextById(aStrings[i], aDefaults[i]), Scaled(0xe6), Scaled(0x1db + 0x30 * i), -1, -1, true, 0, -1, -1);
}

// 0x790030 (bejeweled3), slot 90: the values, right at 0x2f8: the multiplier (string 0x113, "x%d"; the board's +0xe0c), the columns crushed
// (counter 0x23), the best chain and the game's seconds (counter 0) as minutes and seconds (string 0x114, "%d:%02d")
void IceStormEndLevelDialog::DrawStatisticsValues(Graphics* g)
{
	const int aX = Scaled(0x2f8);
	char aMultiplier[32];
	snprintf(aMultiplier, sizeof(aMultiplier), TextById(0x113, "x%d").c_str(), mLevelNumber);
	g->WriteString(aMultiplier, aX, Scaled(0x1db), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mCounters[IceStormBoard::COUNTER_COLUMNS_CRUSHED]), aX, Scaled(0x1db + 0x30), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mBestChain), aX, Scaled(0x1db + 0x60), -1, 1, true, 0, -1, -1);
	char aTime[32];
	snprintf(aTime, sizeof(aTime), TextById(0x114, "%d:%02d").c_str(), mCounters[COUNTER_SECONDS] / 60, mCounters[COUNTER_SECONDS] % 60);
	g->WriteString(aTime, aX, Scaled(0x1db + 0x90), -1, 1, true, 0, -1, -1);
}

// the dialog of 0x6a7c80 (the leaderboard of the mode's name "BuriedTreasure") with the goal's depth and its best treasure: 0x7353d0 looks through the artifacts the
// player collected (the list at +0x1bc) for the one worth most (ArtifactBaseValue times its value), whose name it shows
DigEndLevelDialog::DigEndLevelDialog(DigBoard* theBoard) :
	ClassicEndLevelDialog(theBoard),
	mDepth(theBoard->Goal()->DepthShown())
{
	for (int anIndex : theBoard->Goal()->mCollectedArtifacts) {
		const DigGoal::Artifact& anArtifact = theBoard->Goal()->mArtifacts[(size_t)anIndex];
		const int aValue = theBoard->Goal()->mArtifactBaseValue * anArtifact.mValue;
		if (mBestTreasureValue <= aValue) {
			mBestTreasureValue = aValue;
			mBestTreasureText = anArtifact.mText;
		}
	}
}

// slot 94: the statistics table and the top scores, as Lightning's
void DigEndLevelDialog::DrawContent(Graphics* g)
{
	DrawStatisticsTable(g);
	DrawTopScores(g);
}

// 0x732c90 (bejeweled3), slot 89: Max Depth, Total Time, Best Move and Best Treasure (strings 0xcc to 0xcf), left
void DigEndLevelDialog::DrawStatisticsLabels(Graphics* g)
{
	static const char* const aDefaults[4] = {"Max Depth", "Total Time", "Best Move", "Best Treasure"};
	for (int i = 0; i < 4; i++)
		g->WriteString(TextById(0xcc + i, aDefaults[i]), Scaled(0xe6), Scaled(0x1db + 0x30 * i), -1, -1, true, 0, -1, -1);
}

// 0x7353d0 (bejeweled3), slot 90: the values, right at 0xdc: the depth (string 0xd0, "%s m"), the game's seconds (counter 0; 0xd1, "%d:%02d"), the best move (counter 25;
// 0xd2, "$%s") and the best treasure: its value ("$%s", 0xd4) with the artifact's name beneath ("(%s)", 0xd5), or "No Treasures" (0xd3) with none
void DigEndLevelDialog::DrawStatisticsValues(Graphics* g)
{
	const int aX = Scaled(0x2ee);
	char aText[96];
	snprintf(aText, sizeof(aText), TextById(0xd0, "%s m").c_str(), CommaSeperate(mDepth).c_str());
	g->WriteString(aText, aX, Scaled(0x1db), -1, 1, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), TextById(0xd1, "%d:%02d").c_str(), mCounters[COUNTER_SECONDS] / 60, mCounters[COUNTER_SECONDS] % 60);
	g->WriteString(aText, aX, Scaled(0x1db + 0x30), -1, 1, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), TextById(0xd2, "$%s").c_str(), CommaSeperate(mCounters[COUNTER_BEST_MOVE]).c_str());
	g->WriteString(aText, aX, Scaled(0x1db + 0x60), -1, 1, true, 0, -1, -1);
	if (mBestTreasureText.empty()) {
		g->WriteString(TextById(0xd3, "No Treasures"), aX, Scaled(0x1db + 0x90), -1, 1, true, 0, -1, -1);
		return;
	}
	snprintf(aText, sizeof(aText), TextById(0xd4, "$%s").c_str(), CommaSeperate(mBestTreasureValue).c_str());
	g->WriteString(aText, aX, Scaled(0x1db + 0x90), -1, 1, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), TextById(0xd5, "(%s)").c_str(), mBestTreasureText.c_str());
	g->WriteString(aText, aX, Scaled(0x1db + 0xc0), -1, 1, true, 0, -1, -1);
}

// 0x716ab0 (bejeweled3): the dialog of 0x6a7c80 (the leaderboard of the mode's name "Lightning") with the board's +0x30b0
LightningEndLevelDialog::LightningEndLevelDialog(Board* theBoard, int theBonusSecondsTotal) :
	ClassicEndLevelDialog(theBoard),
	mBonusSecondsTotal(theBonusSecondsTotal)
{
}

// 0x780130 (bejeweled3), slot 94: the statistics table (slot 92) and the top scores (slot 88), each where the dialog's own drawing leaves it:
// no section labels over them and no shift down (Classic's 0x7839f0 has both)
void LightningEndLevelDialog::DrawContent(Graphics* g)
{
	DrawStatisticsTable(g);
	DrawTopScores(g);
}

// 0x780160 (bejeweled3), slot 89: Highest Multiplier, Best Move, Longest Cascade and Total Time (strings 0x1e3 to 0x1e6), left
void LightningEndLevelDialog::DrawStatisticsLabels(Graphics* g)
{
	static const char* const aDefaults[4] = {"Highest Multiplier", "Best Move", "Longest Cascade", "Total Time"};
	for (int i = 0; i < 4; i++)
		g->WriteString(TextById(0x1e3 + i, aDefaults[i]), Scaled(0xe6), Scaled(0x1db + 0x30 * i), -1, -1, true, 0, -1, -1);
}

// 0x7805e0 (bejeweled3), slot 90: the values, right at 0x2ee: the multiplier (string 0x1e7, "x%d"; the board's +0xe0c), the best move and the
// longest cascade (counters 25 and 24, as in Classic's) and the time, the 60 seconds and the time gems' as minutes and seconds ("%d:%02d")
void LightningEndLevelDialog::DrawStatisticsValues(Graphics* g)
{
	const int aX = Scaled(0x2ee);
	char aMultiplier[32];
	snprintf(aMultiplier, sizeof(aMultiplier), TextById(0x1e7, "x%d").c_str(), mLevelNumber);
	g->WriteString(aMultiplier, aX, Scaled(0x1db), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mCounters[COUNTER_BEST_MOVE]), aX, Scaled(0x1db + 0x30), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mCounters[COUNTER_LONGEST_CASCADE]), aX, Scaled(0x1db + 0x60), -1, 1, true, 0, -1, -1);
	const int aSeconds = mBonusSecondsTotal + 0x3c;
	char aTime[32];
	snprintf(aTime, sizeof(aTime), "%d:%02d", aSeconds / 0x3c, aSeconds % 0x3c);
	g->WriteString(aTime, aX, Scaled(0x1db + 0x90), -1, 1, true, 0, -1, -1);
}

// 0x780aa0 (bejeweled3), slot 26: Classic's drawing (0x6a87a0), then DrawResults
void LightningEndLevelDialog::Draw(Graphics* g)
{
	ClassicEndLevelDialog::Draw(g);
	DrawResults(g);
}

// 0x780aa0 (bejeweled3) after the dialog's own drawing: the graph's picture and its lines, then the four icons (the counts of the Flame Gems,
// Star Gems and Hypercubes and the time gems' seconds) and the three coloured boxes (SPEED, SPECIAL, MATCHES), each with its text to the right of
// it at y 0x2d3, and the chart's side labels "1k" to "5k" going up from y 0x3cf, 0x2e apart. Every picture sits where resources.xml puts it with
// the setting LIGHTNING_GAMEOVER_DLG_*_OFFSET (all -160 in defaultUIConstants.xml) added to x.
// port: the chart's bars (stacked, three colours, one for each record the board keeps of its rounds: the exe's +0x36c list, filled by code that
// is not translated) are not drawn: the chart stays empty, its scale at 1k to 5k as the exe's own with no record
void LightningEndLevelDialog::DrawResults(Graphics* g)
{
	g->DrawImage(IMAGE_GAMEOVER_SECTION_GRAPH, Scaled(GetImageOffsetX(IMAGE_GAMEOVER_SECTION_GRAPH_ID) - 0xa0),
		Scaled(GetImageOffsetY(IMAGE_GAMEOVER_SECTION_GRAPH_ID)));
	g->DrawImage(IMAGE_GAMEOVER_LINES, Scaled(GetImageOffsetX(IMAGE_GAMEOVER_LINES_ID) - 0xa0), Scaled(GetImageOffsetY(IMAGE_GAMEOVER_LINES_ID)));

	struct ResultIcon {
		Image* mImage;
		int mImageId;
		const char* mOffsetSetting;
		std::string mText;
	};
	char aFlame[32], aStar[32], aHyper[32], aTime[32];
	snprintf(aFlame, sizeof(aFlame), TextById(0x1e9, "FLAME x%d").c_str(), mCounters[COUNTER_FLAME_GEMS]);
	snprintf(aStar, sizeof(aStar), TextById(0x1ea, "STAR x%d").c_str(), mCounters[COUNTER_STAR_GEMS]);
	snprintf(aHyper, sizeof(aHyper), TextById(0x1eb, "HYPER x%d").c_str(), mCounters[COUNTER_HYPERCUBES]);
	snprintf(aTime, sizeof(aTime), TextById(0x1ec, "TIME +%d:%02d").c_str(), mBonusSecondsTotal / 0x3c, mBonusSecondsTotal % 0x3c);
	const ResultIcon aIcons[7] = {
		{IMAGE_GAMEOVER_ICON_FLAME, IMAGE_GAMEOVER_ICON_FLAME_ID, "LIGHTNING_GAMEOVER_DLG_ICON_FLAME_OFFSET", aFlame},
		{IMAGE_GAMEOVER_ICON_STAR, IMAGE_GAMEOVER_ICON_STAR_ID, "LIGHTNING_GAMEOVER_DLG_ICON_STAR_OFFSET", aStar},
		{IMAGE_GAMEOVER_ICON_HYPERCUBE, IMAGE_GAMEOVER_ICON_HYPERCUBE_ID, "LIGHTNING_GAMEOVER_DLG_ICON_HYPERCUBE_OFFSET", aHyper},
		{IMAGE_GAMEOVER_ICON_LIGHTNING, IMAGE_GAMEOVER_ICON_LIGHTNING_ID, "LIGHTNING_GAMEOVER_DLG_ICON_LIGHTNING_OFFSET", aTime},
		{IMAGE_GAMEOVER_BOX_YELLOW, IMAGE_GAMEOVER_BOX_YELLOW_ID, "LIGHTNING_GAMEOVER_DLG_BOX_YELLOW_OFFSET", TextById(0x1ed, "SPEED")},
		{IMAGE_GAMEOVER_BOX_ORANGE, IMAGE_GAMEOVER_BOX_ORANGE_ID, "LIGHTNING_GAMEOVER_DLG_BOX_ORANGE_OFFSET", TextById(0x1ee, "SPECIAL")},
		{IMAGE_GAMEOVER_BOX_PINK, IMAGE_GAMEOVER_BOX_PINK_ID, "LIGHTNING_GAMEOVER_DLG_BOX_PINK_OFFSET", TextById(0x1ef, "MATCHES")},
	};
	const int aLabelSpacing = Scaled(gApp->GetInteger("LIGHTNING_GAMEOVER_DLG_ICON_LABEL_SPACING", 5));
	g->SetFont(FONT_GAMEOVER_DIALOG_SMALL);   // 0x8d6dfc: the labels' font
	{
	LayerColors aLayers(FONT_GAMEOVER_DIALOG_SMALL);
	aLayers.Set("OUTLINE", Color(255, 255, 255, 255));
	for (const ResultIcon& anIcon : aIcons) {
		const int aOffset = gApp->GetInteger(anIcon.mOffsetSetting, -160);
		const int aX = Scaled(GetImageOffsetX(anIcon.mImageId) + aOffset);
		g->DrawImage(anIcon.mImage, aX, Scaled(GetImageOffsetY(anIcon.mImageId)));
		g->WriteString(anIcon.mText, aX + anIcon.mImage->GetCelWidth() + aLabelSpacing, Scaled(0x2d3), -1, -1, true, 0, -1, -1);
	}
	}
	g->SetFont(FONT_GAMEOVER_DIALOG_MED);   // 0x8d7424: the scale's font
	for (int i = 0; i < 5; i++) {
		char aScale[16];
		snprintf(aScale, sizeof(aScale), TextById(0x1f0, "%dk").c_str(), i + 1);
		g->WriteString(aScale, Scaled(0x14a), Scaled(0x3cf) - Scaled(0x2e) * i, -1, 1, true, 0, -1, -1);
	}
}

// 0x782cf0 (bejeweled3): the dialog of 0x6a7c80 (the leaderboard of the mode's name "Poker") with the board's hands' counts (+0x39e4)
PokerEndLevelDialog::PokerEndLevelDialog(PokerBoard* theBoard) :
	ClassicEndLevelDialog(theBoard)
{
	for (int i = 0; i <= PokerBoard::HAND_KINDS; i++) {
		mHandCounts[i] = theBoard->mHandCounts[i];
		if (i > 0) {
			mHands += mHandCounts[i];
			if (mHandCounts[i] > 0)
				mBestHand = i;
		}
	}
	mSkullsBusted = theBoard->mSkullsBusted;
	mCoinFlips = theBoard->mCounters[PokerBoard::COUNTER_COIN_FLIPS];
}

// 0x782580 (bejeweled3), slot 94: the statistics table (slot 92) and the top scores (slot 88): no panel labels over them and no gems' row
void PokerEndLevelDialog::DrawContent(Graphics* g)
{
	DrawStatisticsTable(g);
	DrawTopScores(g);
}

// 0x782870 (bejeweled3), slot 89: Best Hand, Number of Hands, Skulls Busted and Skull Coin Flips (strings 0x191 to 0x194), left
void PokerEndLevelDialog::DrawStatisticsLabels(Graphics* g)
{
	static const char* const aDefaults[4] = {"Best Hand", "Number of Hands", "Skulls Busted", "Skull Coin Flips"};
	for (int i = 0; i < 4; i++)
		g->WriteString(TextById(0x191 + i, aDefaults[i]), Scaled(0xe6), Scaled(0x1db + 0x30 * i), -1, -1, true, 0, -1, -1);
}

// 0x7825b0 (bejeweled3), slot 90: the four values, right at 0x2f8 (each a text made by 0x43b2e0). port: what the exe passes in its registers is not
// in the export: the best hand is its name, the others are the counts above
void PokerEndLevelDialog::DrawStatisticsValues(Graphics* g)
{
	static const char* const aNames[8] = {"", "Pair", "Spectrum", "2 Pair", "3 of a Kind", "Full House", "4 of a Kind", "Flush"};
	const int aX = Scaled(0x2f8);
	g->WriteString(aNames[mBestHand], aX, Scaled(0x1db), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mHands), aX, Scaled(0x1db + 0x30), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mSkullsBusted), aX, Scaled(0x1db + 0x60), -1, 1, true, 0, -1, -1);
	g->WriteString(CommaSeperate(mCoinFlips), aX, Scaled(0x1db + 0x90), -1, 1, true, 0, -1, -1);
}

// 0x782e10 (bejeweled3), slot 26: Classic's drawing (0x6a87a0), then the chart of the seven kinds' counts where Classic has its row of special gems
// (the exe's chart starts at y 0x29e and writes its labels at y 0x2cb, from x 0x1ae). port: seven bars with the kind's name over each and its count
// under it, placed to fill that row; the exe's axis and bar drawing are not translated (see NOTES.md, Oddities)
void PokerEndLevelDialog::Draw(Graphics* g)
{
	ClassicEndLevelDialog::Draw(g);
	static const char* const aNames[8] = {"", "Pair", "Spec", "2 Pr", "3 Kd", "Full", "4 Kd", "Flush"};
	int aLargest = 1;
	for (int i = 1; i <= PokerBoard::HAND_KINDS; i++)
		aLargest = mHandCounts[i] > aLargest ? mHandCounts[i] : aLargest;
	const int aLeft = Scaled(0x118);
	const int aSlot = Scaled(0x7c);
	const int aBarWidth = Scaled(0x5c);
	const int aBottom = Scaled(0x3a0);
	const int aTall = Scaled(0xa0);
	g->SetFont(FONT_CARD_COUNT_SMALL);
	for (int i = 1; i <= PokerBoard::HAND_KINDS; i++) {
		const int aX = aLeft + (i - 1) * aSlot;
		const int aHeight = aTall * mHandCounts[i] / aLargest;
		g->SetColor(Color(220, 190, 80, 255));
		g->FillRect(aX, aBottom - aHeight, aBarWidth, aHeight);
		g->SetColor(Color(255, 255, 255, 255));
		g->WriteString(aNames[i], aX + aBarWidth / 2, Scaled(0x2c0), -1, 0, true, 0, -1, -1);
		g->WriteString(CommaSeperate(mHandCounts[i]), aX + aBarWidth / 2, Scaled(0x3a8), -1, 0, true, 0, -1, -1);
	}
}

}  // namespace Sexy
