#ifndef __BOOKWORM_DEFINITIONATOR_H__
#define __BOOKWORM_DEFINITIONATOR_H__

// Definitionator.cpp: the word -> definition table from popdefs.txt. The app's loading thread makes it
// (BookWormApp::mDefinitionator) and loads "popdefs.txt"; Board::Submit looks up every played word when
// the definitions option is on and passes a non-empty result to Lex::SayDefinition.
// Original: class Sexy::Definitionator (no base), sizeof 0x14 (new(0x14) in the loading code 0x41a8a0),
// vftable 0x4fc24c (2 slots: 0 GetNumDefinitions, 1 deleting dtor).
// Behaviour spec: re/bookworm/analysis/lex.md section 5.

#include "Common.h"
#include <map>
#include <string>

namespace Sexy
{

class BookWormApp;

class Definitionator
{
public:
	// +0x00 vfptr
	BookWormApp*						mApp;           // +0x04 ctor arg, unused
	std::map<std::string, std::string>	mDefinitions;   // +0x08 (VC7 map: head +0x0c, size +0x10); key = the word as in the file

public:
	Definitionator(BookWormApp* theApp);                            // 0x41dd30
	virtual int		GetNumDefinitions();                            // 0x41d910 slot 0 (never called through the vtable)
	virtual ~Definitionator();                                      // 0x41d8d0 (deleting dtor 0x41dd80) slot 1

	bool			Load(const SexyString& theFileName);            // 0x41d920; true if any definition was read
	SexyString		GetDefinition(const SexyString& theWord);       // 0x41d2b0; "" when absent (exact, case-sensitive)
};

}

#endif
