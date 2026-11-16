#ifndef __FEEDINGFRENZY_BUBBLESTRING_H__
#define __FEEDINGFRENZY_BUBBLESTRING_H__

// The banner a stage ends with (BubbleString, 0x138 bytes, constructor 0x43ea90, vtable 0x4a7920): a line of letters,
// each a bubble that bobs on its own wave.

#include "Node.h"

#include <string>
#include <vector>

namespace feedingfrenzy
{

class BubbleString : public engine::Container
{
public:
	BubbleString();  // 0x43ea90

	void setText(const std::string& theText);  // 0x43eb20
	float getWidth();                           // 0x43e580
	void show(const engine::Point& thePlace);   // 0x43e5f0
	void hide();                                // 0x43e6c0
};

} // namespace feedingfrenzy

#endif
