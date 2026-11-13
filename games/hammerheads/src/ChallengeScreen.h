#ifndef __HAMMERHEADS_CHALLENGESCREEN_H__
#define __HAMMERHEADS_CHALLENGESCREEN_H__

// The challenge (0x44c170-0x44e110): the bonus round a challenge gnome opens. Original: class Sexy::ChallengeScreen : public
// Widget (sizeof 0xb8, made by 0x41e8b0).

#include "Widget.h"

namespace Sexy
{

class ChallengeScreen : public Widget
{
public:
	ChallengeScreen();									// 0x44d800
	virtual ~ChallengeScreen();

	void			Opened(int theArg);					// vfd8 (0x41f180 calls it with 0 right after making the screen)
	void			Closing();							// vfdc (0x421d70 calls it before the screen is deleted)
	void			StartChallenge();					// 0x44cd80: the bonus round dialog was answered
	virtual void	GotFocus() override;				// vf74
	virtual void	LostFocus() override;				// vf78

	int				GetCreatureKind();
};

}

#endif
