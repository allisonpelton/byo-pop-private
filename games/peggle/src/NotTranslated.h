// NotTranslated.h: marks a path of the original that the port has not reached yet. It stops the game with a message naming
// the place, so a run never goes on with something missing. `grep NOT_TRANSLATED games/peggle/src` lists what is left.

#ifndef __NOTTRANSLATED_H__
#define __NOTTRANSLATED_H__

namespace Sexy
{

[[noreturn]] void NotTranslated(const char* theWhat, const char* theFile, int theLine);

} // namespace Sexy

#define NOT_TRANSLATED(what) ::Sexy::NotTranslated(what, __FILE__, __LINE__)

#endif
