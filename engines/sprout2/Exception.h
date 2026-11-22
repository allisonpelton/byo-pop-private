#ifndef __SPROUT2_EXCEPTION_H__
#define __SPROUT2_EXCEPTION_H__

// What the engine throws when a file or a document is wrong (engine::Exception, a std::exception with its message
// at +0x18 (feedingfrenzy2); its copy constructor 0x51c878 (feedingfrenzy2)): a document handler's "TRIED TO OPEN
// EMPTY TAG!" or a screen's "attachPoint with no name!".

#include <exception>
#include <string>

namespace engine
{

class Exception : public std::exception
{
public:
	explicit Exception(const std::string& theMessage) : mMessage(theMessage) {}

	const char* what() const noexcept override { return mMessage.c_str(); }
	const std::string& getMessage() const { return mMessage; }

private:
	std::string mMessage;   // +0x18
};

} // namespace engine

#endif
