#ifndef __PUZZLESTATEOVERLAY_H__
#define __PUZZLESTATEOVERLAY_H__

// PuzzleStateOverlay.h: the dome with the five pods of a puzzle pack's planet, drawn beside the board of a puzzle game
// (Sexy::PuzzleStateOverlay, 0x5d1731-0x5d2fb2; 0x2c8 bytes; and Sexy::PuzzlePod, 0x5d16ad; 0x60 bytes). Board::mPuzzleOverlay
// (+0x90) holds it. Each pod shows a thumbnail of its puzzle and lights up as the puzzle is solved; when all five of a
// planet's pods are lit the dome opens and the warp button shows. The offsets in the comments are the original's, from
// the start of the object.

#include "Widget.h"
#include "ButtonListener.h"
#include "CurvedVal.h"
#include "Point.h"
#include <string>

namespace Sexy
{

class WinDMApp;
class MemoryImage;
class PodButton;

// one of the five pods (0x5d16ad)
class PuzzlePod
{
public:
	int				mIndex;             // +0x08 the place of the pod on the dome (0 to 4)
	CurvedVal		mCurve;             // +0x10 how far from the dome's middle the pod is (91 at 1, 126 at 0)
	float			mStep;              // +0x40 how much mCurve's input moves every update once mDelay is over
	int				mDelay;             // +0x44 updates before the pod starts to move
	MemoryImage*	mThumb;             // +0x48 the thumbnail of the pod's puzzle
	bool			mSolved;            // +0x4c
	int				mHold;              // +0x50 updates before mLit starts to rise once the pod is solved
	float			mLit;               // +0x54 how much of the pod's lit picture shows, 0 to 1
	float			mGlow;              // +0x58 the glow of the pod that is selected

public:
	PuzzlePod();                        // 0x5d16ad
	virtual ~PuzzlePod();               // 0x5d171d

	Point			GetPos();           // 0x5d1bb3 the place of the pod's picture, in the 640 by 480 scale
};

class PuzzleStateOverlay : public Widget, public ButtonListener
{
public:
	WinDMApp*		mApp;               // +0x8c
	MemoryImage*	mDomeImage;         // +0x90 the dome's picture with the mask of how far it is open
	PuzzlePod		mPods[5];           // +0x98
	int				mSelectedPod;       // +0x278 the pod of the puzzle being played, -1
	int				mPodOver;           // +0x27c the pod that the mouse is over, -1
	int				mSolvedCount;       // +0x280 the pods of the planet that are solved, 0 to 5
	int				mDomeDelay;         // +0x284 updates before the dome starts to open
	float			mDomeOpen;          // +0x288 how far the dome is open, 0 to 1
	bool			mDomeDirty;         // +0x28c mDomeImage is made again
	PodButton*		mDomeWidget;        // +0x290 the button over the dome that opens the galaxy map
	std::string		mPlanetName;        // +0x294 the name of the planet of the pack
	MemoryImage*	mTitleImage;        // +0x2b0 the name in the glow font
	float			mPulse;             // +0x2b4 the pulse of the warp button
	int				mDelayA;            // +0x2b8 updates before mFlashA starts
	float			mFlashA;            // +0x2bc the flash of the pods' picture, 0 for none
	int				mDelayB;            // +0x2c0 updates before mFlashB starts
	float			mFlashB;            // +0x2c4 the flash of the vapor

public:
	PuzzleStateOverlay(WinDMApp* theApp);   // 0x5d2fb2
	virtual ~PuzzleStateOverlay();          // 0x5d294b

	void			Setup(bool theKeepPuzzle);                  // 0x5d2cd6 (true when the profile's puzzle number stays)
	void			GenerateThumbnails();                       // 0x5d2a83
	void			RebuildDomeImage();                         // 0x5d1777
	int				GetPodAt(int theX, int theY);               // 0x5d1f46 the pod under the point, -1
	bool			MouseOverDome(int theX, int theY);          // 0x5d18eb
	void			SetPodOver(int thePod);                     // 0x5d1995
	void			SelectPod(int thePod);                      // 0x5d19a5
	void			PuzzleCompleted();                          // 0x5d19b5
	void			DrawBack(Graphics* g);                      // 0x5d1a3b
	void			DrawFront(Graphics* g);                     // 0x5d2004
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5d1731
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5d1754
	virtual void	Update() override;                          // 0x5d1c27
	virtual void	ButtonDepress(int theId) override;          // 0x5d1b5d
};

}

#endif
