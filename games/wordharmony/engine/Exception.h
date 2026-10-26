#ifndef __WORDHARMONY_ENGINE_EXCEPTION_H__
#define __WORDHARMONY_ENGINE_EXCEPTION_H__

// The engine's exception (0x3c bytes, constructor 0x402700): an Object that is also a std::exception and holds a
// message (+0x18). 0x43a860 throws a new one by pointer; the port throws it by value.

#include <exception>
#include <string>

namespace engine
{

class Exception : public std::exception
{
public:
	Exception(const std::string& theMessage) : mMessage(theMessage) {}  // 0x402700
	const char* what() const noexcept override { return mMessage.c_str(); }
	const std::string& getMessage() const { return mMessage; }

private:
	std::string mMessage;
};

} // namespace engine

#endif
