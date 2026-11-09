#ifndef __ASTROPOP_LOADERSCREEN_H__
#define __ASTROPOP_LOADERSCREEN_H__

// LoaderScreen: the loading screen with its progress bar, the "click to play" button and the slide show of the four
// characters (Vector, Sprocket, Turbot, Vixx). Original: constructor 0x45c0f0 (sizeof 0xa0, vftable 0x5648f8, 47 slots:
// 1 AddedToManager 0x445c10, 2 RemovedFromManager 0x434860, 16 Draw 0x445df0, 17 Update 0x43ba50 + 0x43bb40),
// destructor body 0x445b10, slide set-up 0x4548e0.
// Left out (registration and outbound code): the register button (id 1100) at +0x98 and the PopCap link widget at +0x9c.

#include "Widget.h"
#include <vector>

namespace Sexy
{

class AstroPopApp;
class ButtonWidget;
class Image;
class MemoryImage;

// One slide (0x1c bytes): a character's picture and silhouette, a big text strip and its tinted copy
struct LoaderSlide
{
	Image*			mPicture;           // +0x00 IMAGE_LOADER_VECTOR + n (the character, drawn at 453, 147)
	Image*			mSilhouette;        // +0x04 IMAGE_LOADER_VECTOR_SILHOUETTE + n (drawn first, at 339, 169)
	Image*			mBigText;           // +0x08 IMAGE_LOADER_VECTOR_BIGTEXT + n (a strip of three, drawn at y 274)
	MemoryImage*	mTinted;            // +0x0c the big text at half size in the slide's colour (a strip of six, y 236)
	int				mFade;              // +0x10 255 down to 0 by 2 per update: the white flash
	int				mBigTextX;          // +0x14 scroll offset of the big text strip
	int				mTintedX;           // +0x18 scroll offset of the tinted strip
};

class LoaderScreen : public Widget
{
public:
	AstroPopApp*			mApp;               // +0x58
	std::vector<LoaderSlide*> mSlides;          // +0x5c (the vector: begin +0x60, end +0x64)
	int						mCurSlide;          // +0x6c a random slide to begin with
	int						mDirY;              // +0x70 -1 or 1, flips with every slide
	int						mDirX;              // +0x74 1 or -1
	ButtonWidget*			mPlayButton;        // +0x94 id 2200

	LoaderScreen(AstroPopApp* theApp);          // 0x45c0f0
	virtual ~LoaderScreen();                    // 0x445b10

	void			MakeSlides();               // 0x4548e0 (called by Init after the constructor)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;     // 0x445c10
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x434860
	virtual void	Draw(Graphics* g) override;       // 0x445df0
	virtual void	Update() override;                // 0x43ba50
};

}

#endif
