#ifndef __WINDMAPP_H__
#define __WINDMAPP_H__

// WinDMApp.cpp: the application class (a stub until the port of the original's class goes in).

#include "SexyApp.h"

namespace Sexy
{

class WinDMApp : public SexyApp
{
public:
	WinDMApp();
	virtual ~WinDMApp();
	virtual void Init() override;
};

}

#endif
