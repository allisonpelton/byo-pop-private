#ifndef __BOOKWORM_CREDITSDIALOG_H__
#define __BOOKWORM_CREDITSDIALOG_H__

// CreditsDialog.cpp: the credits (dialog 16, from the Options dialog): six headings and names over a gold
// panel with drifting and rotating clouds, and a footer button (2016, handled by the app).
// Original: class Sexy::CreditsDialog : public Dialog, sizeof 0x138 (new in BookWormApp::DoCredits
// 0x4106c0), vftable 0x4fc07c (52 slots), ButtonListener vftable 0x4fc060 (+0x58). Offsets are absolute
// (the 2004 Dialog is 0x110 bytes). Behaviour spec: re/bookworm/analysis/screens.md. The port derives it
// from Dialog2004 (the 2004 dialog buttons and drawing, Compat2004.h).

#include "Compat2004.h"

namespace Sexy
{

class BookWormApp;
class Graphics;
class ImageFont;

class CreditsDialog : public Dialog2004
{
public:
	enum
	{
		PREFERRED_HEIGHT = 396
	};

	BookWormApp*	mApp;                   // +0x110
	// +0x114 pad
	double			mCloudRot;              // +0x118 rotation of the top cloud layer (radians), += 0.004
	double			mCloudX;                // +0x120 horizontal cloud scroll, += 0.2, -512 when > 0
	double			mCloudY;                // +0x128 vertical cloud scroll, += 0.3, -512 when > 0
	ImageFont*		mFont;                  // +0x130 LegacySerifMdSCITCTT12 (the credits lines)
	// +0x134 pad (sizeof 0x138)

public:
	CreditsDialog(BookWormApp* theApp);                             // 0x41c0b0
	virtual ~CreditsDialog();                                       // 0x41bfc0 (scalar deleting dtor 0x41c080) slot 0
	virtual void	Draw(Graphics* g) override;                     // 0x41c2a0 slot 16
	virtual void	Update() override;                              // 0x41bff0 slot 17
	virtual int		GetPreferredHeight(int theWidth) override;      // 0x41bfe0 slot 50 (constant 396)
};

}

#endif
