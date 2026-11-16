// ThunderballApp.h: Peggle's application class (the original's Sexy::ThunderballApp, a global object whose
// constructor runs before WinMain; WinMain calls Init, Start and Shutdown on it).

#ifndef __THUNDERBALLAPP_H__
#define __THUNDERBALLAPP_H__

#include "SexyApp.h"

namespace Sexy
{

class ThunderballApp : public SexyApp
{
public:
	ThunderballApp(); // 0x426f10
	~ThunderballApp() override;

	void Init() override; // 0x42ca30
};

extern ThunderballApp* gThunderballApp;

} // namespace Sexy

#endif
