#ifndef __BACKDROPSPARSER_H__
#define __BACKDROPSPARSER_H__

// BackdropsParser.h: reads properties\backdrops.xml (Sexy::BackdropsParser, 0x593713-0x59413d): for each of the 50
// backdrops the colors that tint the user interface and the gems.

#include <string>

namespace Sexy
{

class WinDMApp;
class XMLParser;

class BackdropsParser
{
public:
	WinDMApp*		mApp;               // +0x04
	XMLParser*		mParser;            // +0x08
	bool			mHasFailed;         // +0x0c
	std::string		mErrorText;         // +0x10

public:
	BackdropsParser(WinDMApp* theApp);
	~BackdropsParser();

	bool			Parse(const std::string& theText);
	void			Fail(const std::string& theError);                  // 0x593732
	bool			ParseEnd();                                         // 0x59387a
	bool			ParseBackdrop(int theId);                           // 0x5939a8
	bool			ParseBackdrops();                                   // 0x593e5e
};

}

#endif
