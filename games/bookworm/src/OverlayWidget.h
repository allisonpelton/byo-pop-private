#ifndef __BOOKWORM_OVERLAYWIDGET_H__
#define __BOOKWORM_OVERLAYWIDGET_H__

// OverlayWidget.cpp: the board's top layer. It owns an effect manager (the flying tiles, drawn first),
// the bonus-word banner sliding up from the bottom with its pulsing arrows, the bonus word's thought
// balloon (GT_BONUS_WORD_BALLOON_FORMAT) shown while the mouse is over the banner, Lex's sweat drops, the
// pause box, the fire column above a burning tile and the white flash when the board burns. It never takes
// the mouse.
// Original: class Sexy::OverlayWidget : public Widget, sizeof 0x90 (new in Board::AddedToManager 0x403730,
// Board::mOverlay), vftable 0x500cbc (47 slots). Offsets are absolute (the 2004 Widget is 0x58 bytes).
// Behaviour spec: re/bookworm/analysis/screens.md.

#include "Widget.h"
#include "Rect.h"

#include <list>
#include <cstdio>

namespace Sexy
{

class BookWormApp;
class Graphics;
class WidgetManager;
class EffectMgr;
class Effect;
class WordBalloon;

class OverlayWidget : public Widget
{
public:
	enum
	{
		BALLOON_WIDTH = 213                 // the shared balloon's width (its height comes from GetPreferredHeight)
	};

	BookWormApp*	mApp;                   // +0x58
	EffectMgr*		mEffectMgr;             // +0x5c new EffectMgr(app); Board::Submit adds the flying tiles to it
	WordBalloon*	mBalloon;               // +0x60 Lex's balloon (Say/Think), created in AddedToManager
	bool			mShowBalloon;           // +0x64 request: open the bonus-word balloon on the next update
	bool			mBalloonUp;             // +0x65 the balloon shows the bonus-word text
	int				mBannerTargetY;         // +0x68 banner target y: 406 shown, the board's height hidden (Board::ShowBonusWord)
	int				mBannerY;               // +0x6c banner y, 0 = never placed (Board::ShowBonusWord sets the board height first)
	bool			mBannerActive;          // +0x70 banner drawn and hoverable (set by Board; cleared when a slide down overshoots)
	// +0x71..+0x77 pad
	double			mArrowFade;             // +0x78 arrow alpha (>1 = opaque); Board::ShowBonusWord sets 10.0, -0.025 per update
	Rect			mBannerRect;            // +0x80 hover area (10, mBannerY, banner width, banner height)

public:
	OverlayWidget(BookWormApp* theApp);                             // 0x430c20
	virtual ~OverlayWidget();                                       // 0x430970 (scalar deleting dtor 0x430ce0) slot 0
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;  // 0x430d10 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x430bb0 slot 2
	virtual void	OrderInManagerChanged() override;               // 0x430b90 slot 3
	virtual void	Draw(Graphics* g) override;                     // 0x431080 slot 16
	virtual void	Update() override;                              // 0x430e20 slot 17

	bool			IsBonusWordMoving();                               // 0x430a00
	void			AddEffect(Effect* theEffect);                   // 0x430a20
	std::list<Effect*>* GetEffects(int theType);                    // 0x430a70 (new list, the caller deletes it; NULL when none)
	bool			HasEffects(int theType);                        // 0x430a90
	void			SaveState(FILE* theFile);                       // 0x430ab0 (33 bytes, then the effects)
	void			LoadState(FILE* theFile);                       // 0x430b20
};

}

#endif
