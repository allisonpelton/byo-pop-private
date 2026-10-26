// Sexy::Credits (0x158 bytes, constructor 0x434ef0; vtables 0x7abffc (its JeffLib::DataDrivenClass part, at +0),
// 0x7ac00c (the Widget, at +0x60) and 0x7ac128 (the PopAnimListener, at +0xe8); code 0x434dc0-0x440e80): the
// credits screen, which the options' Credits button shows and the end of the game ends with. It is read from
// data/credits.xml, a list of credits. Each has its settings (a heading, the background: a PAM animation that
// fades in over the black or a picture, how long to wait once everything has faded in, how fast it fades) and
// its entries: either a name with a creature (a PAM animation) that fights the name, or the columns of names of
// the thanks. The window is a band of the screen (y 195 to 410) that opens from its middle.
//
// A credit goes like this: the heading fades in over the background; when it is clear enough (nextalpha) the
// first name fades in under it, and every name that has faded in far enough lets the next begin. Once everything
// is in, the creatures wait idlecount updates and then play the attack their entries name (an entry that names none
// stays idle). The commands of their animations act on the screen: "slide" moves a creature, "causedamage" shakes
// everything. When the attacks are over the names fly at their creatures one after the other, and a creature dies
// when its name has flown as far as targetx. When all are dead the credit waits, fades to black and the next
// begins. After the last one the curtain closes and GameApp::CloseCredits ends the screen, but at the end of the
// game (mEndOfGame) Lex comes in first, a row of tiles thanks the player and Lex bows out in a flash of white
// (UpdateFinale).
//
// Made and removed by GameApp (0x450d80 and 0x450e90, GameAppCredits.cpp).
//
// port: the original read the XML with JeffLib::DataDrivenClass's loader (jefflib/DataDrivenClass.h) and so does
// this; the reload hooks (slots 0 and 1) are ported, but nothing in the game calls them.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "DataDrivenClass.h"
#include "PopAnim.h"
#include "Widget.h"

namespace Sexy {

class GameApp;
class Graphics;
class Image;

class Credits : public JeffLib::DataDrivenClass, public Widget, public PopAnimListener {
public:
	// A column of the thanks' names (0x14 bytes)
	struct Column {
		std::vector<std::string> mNames;  // +0x04
		int mWidth = 0;                   // +0x10: the widest of the names
	};

	// A name with its creature (0x8c bytes)
	struct Entry {
		std::string mName;          // +0x00
		std::string mAttack;        // +0x1c: the label of the creature's animation that attacks (none: no attack)
		std::string mPamFile;       // +0x38: "images/pam/<pam>.pam"
		PopAnim* mAnim = nullptr;   // +0x54: the creature
		int mFadeIn = 1;            // +0x58: alpha added an update
		int mNextAlpha = 255;       // +0x5c: the alpha at which the next entry may fade in
		int mAlpha = 0;             // +0x60
		int mX = 0;                 // +0x64: where the creature is drawn (and slides from)
		int mY = 0;                 // +0x68
		int mSlid = 0;              // +0x6c: how far it has slid
		int mSlideSpeed = 0;        // +0x70: pixels an update (negative: away first, then back); 0: none
		int mSlideDistance = 0;     // +0x74: how far it slides before it turns
		int mNameX = 0;             // +0x78: the name's offset (it flies at the creature; it shakes with the screen)
		int mNameY = 0;             // +0x7c
		int mTargetX = 0;           // +0x80: the creature dies when the name has flown this far
		bool mSlideStarted = false; // +0x84: its "slide" command came
		bool mNextStarted = false;  // +0x85: it faded in far enough for the next entry to begin
		bool mShown = false;        // +0x86: it has begun to fade in
		bool mNameFlies = false;    // +0x87: its name has begun to fly at the creature
		bool mDying = false;        // +0x88: the name flew far enough
		bool mDamaged = false;      // +0x89: "causedamage" came
		bool mDone = false;         // +0x8a: nothing more for it to do
	};

	// One credit (0xbc bytes)
	struct Section {
		std::vector<Column> mColumns;  // +0x04
		std::vector<Entry> mEntries;   // +0x10
		std::string mTitle;            // +0x20
		std::vector<int> mDrawOrder;   // +0x3c: the entries, by priority (the higher the later drawn)
		PopAnim* mAnim = nullptr;      // +0x4c: the background animation
		std::string mFrame;            // +0x50: the label of it that plays (and loops)
		std::string mPamFile;          // +0x6c: "images/pam/<bgpam>.pam"
		Image* mBgImage = nullptr;     // +0x88
		int mColumnsX = 0;             // +0x8c: where the columns begin, to centre them
		int mIdleCount = 0;            // +0x90: updates after all has faded in until the creatures attack
		int mAlpha = 0;                // +0x94: the heading and the background fading in
		int mFadeIn = 1;               // +0x98
		int mNextAlpha = 255;          // +0x9c: the alpha at which the first entry begins
		int mY = 185;                  // +0xa0: where the background is drawn
		int mAttackers = 0;            // +0xa4: creatures that attack and have not finished
		int mShakeTime = 200;          // +0xa8: updates the screen shakes for
		int mWait = 100;               // +0xac: updates the credit waits at its end
		int mColumnsHeight = 0;        // +0xb0
		bool mWaited = false;          // +0xb4: the wait at its end is over
		bool mTextShown = false;       // +0xb5: the heading is clear enough for the first entry
		bool mNoEntries = false;       // +0xb6: (nothing reads it)
		bool mShaking = false;         // +0xb7
		bool mAllAttacked = false;     // +0xb8: the last attacker finished its attack
		bool mTextOverCreatures = true;  // +0xb9: once none attacks, the text is drawn over the creatures (not while one slides)
		bool mNamesFly = false;        // +0xba: the attacks are over: the names have begun to fly
	};

	// A letter of the thanks at the end of the game (0x30 bytes)
	struct Tile {
		float mX = 0;          // +0x00
		float mY = 0;          // +0x04
		float mTop = 0;        // +0x08: it bobs between this y
		float mBottom = 0;     // +0x0c: and this one
		float mSpeed = 0;      // +0x10: pixels an update; negative goes up
		std::string mLetter;   // +0x14
	};

	// mState: the end of the game's finale (0: none)
	enum {
		STATE_NONE = 0,
		STATE_LEX_COMES = 1,   // Lex slides down the screen
		STATE_TILES = 2,       // the tiles fade in and bob
		STATE_LEX_ATTACKS = 3, // Lex attacks: the screen flashes white
		STATE_ENDING = 4,      // the curtain closes
	};

	std::vector<std::pair<int, int> > mPriorities;  // +0xf0: (priority, index) of the credit being read's entries
	std::vector<Tile> mTiles;           // +0x100
	std::vector<Section> mSections;     // +0x110
	Section* mCurSection;               // +0x11c
	PopAnim* mLex;                      // +0x120
	int mSectionIndex;                  // +0x124
	int mNextAnimId;                    // +0x128: the id of the next animation made
	int mFadeAlpha;                     // +0x12c: the black the credit fades out to and in from
	int mFadeStep;                      // +0x130: how it changes an update
	int mClipY;                         // +0x134: the band the credit shows in
	int mClipHeight;                    // +0x138
	int mState;                         // +0x13c
	int mLexX;                          // +0x140
	int mLexY;                          // +0x144
	int mTileAlpha;                     // +0x148
	int mFinaleTime;                    // +0x14c
	int mFlashAlpha;                    // +0x150
	bool mFlash;                        // +0x154: the white flash is on
	bool mFading;                       // +0x155: the credit fades out (and the next in)
	bool mOpening;                      // +0x156: the band opens
	bool mEndOfGame;                    // +0x157: the credits of the end of the game: with the finale, then the main menu

	Credits();                          // 0x434ef0
	~Credits() override;                // 0x435410 (0x435440)

	// DataDrivenClass: the data is read again (they run, in this order, around Load; nothing calls them)
	void BeforeReload() override;       // 0x435b40
	void AfterReload() override;        // 0x434dc0 (0x435760)

	// Widget
	void Update() override;                              // 0x436ec0
	void UpdateF(float theFrac) override;                // 0x437c20
	void Draw(Graphics* g) override;                     // 0x438220
	// a click anywhere (the middle button does nothing) ends the credits
	void MouseUp(int x, int y, int theLastDownButtonId) override;  // 0x437cc0

	// PopAnimListener
	void PopAnimStopped(int theId) override;             // 0x437d90
	void PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam) override;  // 0x437fe0

private:
	int mReloadSection;  // port: a global in the original (0x82594c): the credit that was showing when the data was reloaded

	void FreeAnims();    // 0x4355b0: the animations go, and the credits read
	// port: the animations of the credit that begins loaded (0x435876 in AfterReload and 0x437834 in Update)
	void LoadSectionAnims();

	// The tags of the file
	void StartCredit();                                       // 0x435b60: a new credit
	void EndCredit();                                         // 0x435d10: its entries ordered, its columns centred
	void ReadSettings(const AttributeList& theAttributes);    // 0x435ee0
	void ReadEntry(const AttributeList& theAttributes);       // 0x436900: a name with its creature
	void StartColumn();                                       // 0x436650
	void EndColumn();                                         // 0x436790: its width and the height
	void ReadColumnEntry(const AttributeList& theAttributes); // 0x436520: a name of the column

	void DrawCreatures(Graphics* g);   // 0x438810: those that have come, by priority
	void DrawText(Graphics* g);        // 0x4388f0: the heading, the names that fly and the columns
	void UpdateTiles();                // 0x438c30: the tiles bob
	void UpdateFinale();               // 0x438d40: Lex comes in at the end of the game
	void MakeTiles();                  // 0x438f50: the letters of the thanks

	Credits(const Credits&);
	Credits& operator=(const Credits&);
};

}  // namespace Sexy
