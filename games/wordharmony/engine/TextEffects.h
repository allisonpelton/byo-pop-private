#ifndef __WORDHARMONY_ENGINE_TEXTEFFECTS_H__
#define __WORDHARMONY_ENGINE_TEXTEFFECTS_H__

// Floating texts made from named templates (the controller's +0x70; a map of templates at +0xa8). Not ported yet:
// 0x4289d0 makes a text (0x114 bytes, constructor 0x42a890, vtable 0x483a50) from a template, with its ranges for
// the position, the scale and the time drawn at random (0x42bd10).

#include "Node.h"
#include <string>

namespace engine
{

class TextEffect : public Container
{
public:
	// 0x42a7a0
	void setText(std::string theText);
};

class TextEffects
{
public:
	// 0x4289d0: a text of the template theName at (x, y), or null; theDone is called when it ends
	TextEffect* create(std::string theName, float x, float y, void (*theDone)(int));
};

} // namespace engine

#endif
