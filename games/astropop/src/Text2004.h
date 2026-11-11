#ifndef __ASTROPOP_TEXT2004_H__
#define __ASTROPOP_TEXT2004_H__

// The game's widgets write their text with the 2004 framework's writers (Widget slots 36 to 38: WriteString, WriteWordWrapped,
// GetWordWrappedHeight), which the framework's Widget does with SAF 1.3's: a trailing newline adds a line there and not here
// (the "Enter your name:" box is 13 pixels shorter in the original than in 1.3), a wrap at a space keeps one space of a double
// space, and a '^' near the end of a string ends it. Compat2004.h has the 2004 bodies; a widget class takes them by deriving
// from Text2004<Widget> (or <Dialog>) instead of the plain class. docs/FRAMEWORK_2004.md, section 1.4.

#include "Widget.h"
#include "Compat2004.h"

namespace Sexy
{

template <class T>
class Text2004 : public T
{
public:
	using T::T;

	virtual int WriteString(Graphics* g, const SexyString& theString, int theX, int theY, int theWidth = -1,
							int theJustification = -1, bool drawString = true, int theOffset = 0, int theLength = -1) override
	{
		return WriteString2004(g, theString, theX, theY, theWidth, theJustification, drawString, theOffset, theLength);
	}

	virtual int WriteWordWrapped(Graphics* g, const Rect& theRect, const SexyString& theLine, int theLineSpacing,
								 int theJustification) override
	{
		return WriteWordWrapped2004(g, theRect, theLine, theLineSpacing, theJustification);
	}

	virtual int GetWordWrappedHeight(Graphics* g, int theWidth, const SexyString& theLine, int theLineSpacing) override
	{
		return GetWordWrappedHeight2004(g, theWidth, theLine, theLineSpacing);
	}
};

}

#endif
