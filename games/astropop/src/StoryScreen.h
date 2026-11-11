#ifndef __ASTROPOP_STORYSCREEN_H__
#define __ASTROPOP_STORYSCREEN_H__

// StoryScreen: the story between two levels of the warp screen (original class of 800 bytes, constructor 0x4567d0,
// destructor body 0x447190, vftable 0x564ed8, button listener 0x564eb8). Black bars open over the screen from top and
// bottom and between them comes a page of the pilot's story: a picture that plays (the animation of its frames, a patch
// of static or the gauge over it) with its lines of text beside it, then a second page below it, and a button that
// takes the screen away. When the four pilots have been through all their stories, the last story of the pilot is
// followed by the finale: four scenes of three pictures each with their texts, and then all four pilots in one
// picture. Offsets in the comments are the original's.

#include "Text2004.h"
#include "ButtonListener.h"
#include "Color.h"
#include "FrameAnim.h"
#include "Widget.h"
#include <string>
#include <vector>

namespace Sexy
{

class AstroButton;
class AstroPopApp;
class Board;
class Image;

// One line of a page (class of 0x34 bytes, constructor 0x4374d0): the text, which fades in
struct StoryLine
{
	std::string		mText;          // +0x00 may begin with a colour code (^RRGGBB^)
	int				mGap;           // +0x1c extra space under the line
	int				mAlpha;         // +0x20 0 until the line comes, then up to 255
	Color			mColor;         // +0x24 white

	StoryLine() : mGap(0), mAlpha(0), mColor(255, 255, 255, 255) {}
};

// One page of a story (class of 0x74 bytes, constructor 0x44f580, destructor body 0x44b9d0): its picture (the frames
// of an animation, or one picture of several cels) and its lines
class StoryPage
{
public:
	std::vector<StoryLine*>	mLines;         // +0x04
	std::vector<Image*>		mImages;        // +0x14 the frames of the picture (not owned)
	FrameAnim				mAnim;          // +0x20 which frame
	int						mAlpha;         // +0x60 the picture fades in
	int						mSide;          // +0x64 -1: the picture is on the left, 1: on the right
	int						mLevel;         // +0x68 the level of the pilot before which the story comes
	int						mBorder;        // +0x6c the thickness of the frame around the picture
	bool					mStatic;        // +0x70 the picture is overlaid with static
	bool					mGauge;         // +0x71 the picture has the gauge over it

	StoryPage();
	~StoryPage();
};

// The two pages of a story (8 bytes)
struct StoryEntry
{
	StoryPage*		mPageA;
	StoryPage*		mPageB;
};

class StoryScreen : public Text2004<Widget>, public ButtonListener
{
public:
	enum
	{
		BUTTON_ID_NEXT = 0xfb5
	};

	// One of the three pictures of a scene of the finale
	struct ScenePicture
	{
		FrameAnim				mAnim;          // the frame
		int						mSide;          // -1: on the left, 1: on the right
		std::vector<Image*>		mImages;        // the frames (not owned)

		ScenePicture() : mSide(0) {}
	};

	AstroButton*	mButton;            // +0x5c id 0xfb5 (NULL once it has been pressed for the last time)
	AstroPopApp*	mApp;               // +0x60
	Board*			mBoard;             // +0x64
	FrameAnim		mStaticAnim;        // +0x68 the frames of the static over a picture (stopped until a page asks for it)
	FrameAnim		mGaugeAnim;         // +0xa8 the frames of the gauge over a picture
	FrameAnim		mActorAnim[4];      // +0xe8 (Vector), +0x128 (Sprocket), +0x168 (Turbot), +0x1a8 (Vixx): the three
										// frames of each pilot's picture in the last scene
	StoryPage*		mPageA;             // +0x1e8 the upper page
	StoryPage*		mPageB;             // +0x1ec the lower page
	ScenePicture	mScenePicture[3];   // +0x1f0, +0x244, +0x298 the pictures of the scene of the finale
	int				mNumber;            // +0x2ec the number of the story (from 1)
	int				mMode;              // +0x2f0 the pilot
	int				mTotal;             // +0x2f4 how many stories the pilot has (9 for the first pilot, 8 for the others)
	int				mTopBar;            // +0x2f8 how far the black bar above has come (the bars grow a pixel every two updates)
	int				mBottomBar;         // +0x2fc the same for the bar below
	int				mFadeOut;           // +0x300 how white the screen has gone as it goes
	int				mScene;             // +0x304 0: the story, 1 to 4: the scenes of the finale, 5: all the pilots
	int				mSceneAlpha;        // +0x308 the scene fades in (255 for the story)
	int				mTimer;             // +0x30c updates to wait before the second page comes (100)
	bool			mSeen;              // +0x310 the story has been seen before: the button comes as soon as the bars are open
	bool			mDoneA;             // +0x311 the upper page has come in full
	bool			mDoneB;             // +0x312 the lower page has come in full
	bool			mClosing;           // +0x313 the screen goes white
	bool			mFinale;            // +0x314 this story is followed by the finale
	Image*			mBracket;           // +0x318 the frame at the top of the story (not owned)
	Image*			mBracketFlipped;    // +0x31c the same turned over, for the bottom

public:
	// theNumber is the number of the story from 1 and theMode the pilot; theSeen says that the player has seen it before
	StoryScreen(AstroPopApp* theApp, int theNumber, int theMode, bool theSeen, bool theFinaleAnyway);   // 0x4567d0
	virtual ~StoryScreen();                                                     // 0x44cd50 (body 0x447190)

	void			SetPageA(StoryPage* thePage);                               // 0x435b60
	void			SetPageB(StoryPage* thePage);                               // 0x435bb0

	virtual void	AddedToManager(WidgetManager* theManager) override;         // 0x435c00 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;     // 0x435c30 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x435c60 slot 13
	virtual void	Draw(Graphics* g) override;                                 // 0x447400 slot 16
	virtual void	Update() override;                                          // 0x469140 slot 17
	virtual void	MouseDown(int x, int y, int theClickCount) override;        // 0x43cdb0 slot 27
	virtual void	ButtonPress(int theId) override;                            // 0x435cc0
	virtual void	ButtonDepress(int theId) override;                          // 0x456d90

	using ButtonListener::ButtonPress;
	using Widget::MouseDown;
	using Widget::Resize;

private:
	void			CloseButton();                                              // the button goes (part of 0x456d90)
	void			StartScene(int theScene);                                   // the pictures of a scene (part of 0x456d90)
	void			DrawFinale(Graphics* g);                                    // part of 0x447400
	void			DrawStory(Graphics* g);                                     // part of 0x447400
};

}

#endif
