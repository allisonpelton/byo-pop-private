#ifndef __HIGHSCORENAMEDIALOG_H__
#define __HIGHSCORENAMEDIALOG_H__

// HighScoreNameDialog.h: the dialog that asks for the name of a new high score (0x5d3382). TODO: a stub.

#include "PodDialog.h"
#include "EditWidget.h"

namespace Sexy
{

class HighScoreNameDialog : public PodDialog
{
public:
	EditWidget*		mEdit;              // +0x18c
};

}

#endif
