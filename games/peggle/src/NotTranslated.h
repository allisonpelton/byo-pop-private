// NotTranslated.h: marks a path of the original that the port has not reached yet. It stops the game with a message naming
// the place, so a run never goes on with something missing. `grep NOT_TRANSLATED games/peggle/src` lists what is left.

#ifndef __NOTTRANSLATED_H__
#define __NOTTRANSLATED_H__

namespace Sexy
{

[[noreturn]] void NotTranslated(const char* theWhat, const char* theFile, int theLine);

} // namespace Sexy

// Set by a test that looks at the rules only: a drawing function the port has not reached then does nothing (see NOT_TRANSLATED_DRAWING).
namespace Sexy
{
extern bool gSkipUntranslatedDrawing;
}
#define NOT_TRANSLATED_DRAWING(what) 	do { if (!::Sexy::gSkipUntranslatedDrawing) ::Sexy::NotTranslated(what, __FILE__, __LINE__); } while (0)
#define NOT_TRANSLATED(what) ::Sexy::NotTranslated(what, __FILE__, __LINE__)

#endif
