#ifndef __SPROUT2_QDPARSER_H__
#define __SPROUT2_QDPARSER_H__

// The engine's XML parser (engine::QDParser, its constructor 0x4a1760 (feedingfrenzy2)): Sprout's C++ translation of
// Steven Brandt's public-domain "Quick and Dirty" parser, a state machine over one character at a time that calls a
// document handler as elements open and close. An error goes to the log with its line and column, and the parse
// stops there.

#include "Object.h"

namespace engine
{

class DocHandler;
class Reader;

class QDParser : public Object
{
public:
	// 0x448fc0 (feedingfrenzy2), its loop 0x4490c0 (feedingfrenzy2): true when the document ended, false after an
	// error
	static bool parse(DocHandler* theHandler, Reader* theReader);
};

} // namespace engine

#endif
