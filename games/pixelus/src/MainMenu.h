#ifndef __PIXELUS_MAINMENU_H__
#define __PIXELUS_MAINMENU_H__

// MainMenu.cpp: the first screen after the loading screen (Pixelus.exe 0x40ac30 constructor, Draw 0x40a8b0, Update
// 0x40a4b0, button handler 0x40a490, factory 0x40b1c0; vftable 0x50c680). Nuclide's game-selector base (0x4e60e0,
// Update 0x4e5770, Draw 0x4e5da0) draws the picture and a tooltip for the button under the mouse; the game adds the
// hair, sea and sparkle animations, the face that smiles at Play, More Games and Options and frowns at Quit, the
// welcome text with the player's name, and the link to change the player.

#include "Widget.h"
#include "ButtonListener.h"
#include "Effects.h"
#include <string>

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class HyperlinkWidget;

class MainMenu : public Widget, public ButtonListener
{
public:
	enum
	{
		ID_PLAY = 5,
		ID_OPTIONS = 3,
		ID_QUIT = 4,
		ID_USER_LINK = 0x65,
		FACE_X = 477,				// where the face overlay goes (0x1dd, 0x12)
		FACE_Y = 18,
		TOOLTIP_BASELINE = 0x1dc	// the baseline of the tooltip text (+0x158)
	};

	PixelusApp*			mApp;				// +0x80
	DialogButton*		mPlayButton;		// +0x5c
	DialogButton*		mOptionsButton;		// +0x68
	DialogButton*		mQuitButton;		// +0x6c
	HyperlinkWidget*	mUserLink;			// +0x160 "( IF THIS IS NOT YOU, CLICK HERE )"
	EffectList			mEffects;			// +0x164
	int					mFaceCurrent;		// +0x174 the image index of the face being shown, -1 none
	int					mFaceTarget;		// +0x178 the one the mouse asks for
	double				mFaceAlpha;			// +0x180
	const std::string*	mTooltip;			// +0x7c the text of the hovered button
	int					mTooltipAlpha;		// +0x74

public:
	MainMenu(PixelusApp* theApp);			// 0x40ac30
	virtual ~MainMenu();					// 0x40a600

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x40a3b0 slot 1
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x40a440 slot 2
	virtual void		Draw(Graphics* g) override;										// 0x40a8b0 slot 16
	virtual void		Update() override;												// 0x40a4b0 slot 17
	virtual void		ButtonDepress(int theId) override;								// 0x40a490

	void				UserChanged();		// shows or hides the link to change the player

private:
	void				DrawTooltip(Graphics* g);										// 0x4e5da0
	void				UpdateTooltip();												// 0x4e5770
};

}

#endif
