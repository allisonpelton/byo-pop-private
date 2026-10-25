#ifndef __PLATFORM_TEXT_H__
#define __PLATFORM_TEXT_H__

// Text encodings: the games' strings are narrow Latin-1 (Windows-1252 in the originals); SDL, the browser,
// mod.json and the port's JSON files use UTF-8. And game paths as text, for code that links no file system.

#include <cstdint>
#include <string>
#include <vector>

namespace Platform
{

// The code points of UTF-8 text; a malformed sequence becomes U+FFFD
std::vector<uint32_t> DecodeUtf8(const std::string& theText);
void AppendUtf8(std::string* theText, uint32_t theCodePoint);

// Latin-1 from UTF-8: code points above U+00FF and malformed sequences become theReplacement, or are dropped
// when it is 0
std::string Utf8ToLatin1(const std::string& theText, char theReplacement = 0);
std::string Latin1ToUtf8(const std::string& theText);

// A game path as the VFS keys it (VFS::Normalize): '/' for '\\', no empty or "." parts, "a/.." collapsed (a ".."
// with nothing before it kept). Pure string code: the headless tools use it without VFS.cpp.
std::string NormalizePath(const std::string& thePath);

}

#endif
