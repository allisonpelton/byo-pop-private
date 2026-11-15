#ifndef __HAMMERHEADS_LEVELINTROWIDGET_H__
#define __HAMMERHEADS_LEVELINTROWIDGET_H__

// The page a classic level starts with when it introduces a gnome: the gnome on a turning light, a heading and a few lines of text
// about it (the level's file gnomedata\<level>.txt: a number that picks the gnome, then the heading and the lines), and a button
// under them. The page slides down from above the screen, and goes down again when the button is pressed; the game goes on then.
// Original: class Sexy::LevelIntroWidget : public NcMovingWidget, public ButtonListener (0xc0 bytes; the constructor is 0x457f80,
// the destructor 0x457cd0).

#include <string>
#include <vector>

#include "ButtonListener.h"
#include "NcMovingWidget.h"

namespace Sexy
{

class Creature;
class GameScreen;
class NcButton;

class LevelIntroWidget : public NcMovingWidget, public ButtonListener
{
public:
	NcButton*		mButton;			// +0x9c: "continue" (id 0)
	double			mTime;				// +0xa0: runs up while the gnome moves (the light turns with it)
	std::vector<std::string> mLines;	// +0xa8: the heading, then the lines of text
	int				mIntroIndex;		// +0xb8: the number the file starts with; -1 until it has been read
	Creature*		mCreature;			// +0xbc: the gnome on the page

public:
	explicit LevelIntroWidget(int theLevel);										// 0x457f80
	virtual ~LevelIntroWidget();													// 0x457cd0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x457810
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x457840
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x4577b0
	virtual void	Update() override;											// 0x457950
	virtual void	Draw(Graphics* g) override;									// 0x4579c0
	virtual void	Closed() override;											// 0x457910
	virtual void	ButtonDepress(int theId) override;							// 0x4578f0 (the ButtonListener part)

private:
	void			ReadFile(int theLevel);										// 0x457dc0
};

// 0x4580b0: the page of the level being played (the screen's level less one), or null when the mode is a marathon or a tough cookie
// game or when the level has no file
LevelIntroWidget* MakeLevelIntro(GameScreen* theGameScreen);

}

#endif
