#ifndef __WEBCURSOR_H__
#define __WEBCURSOR_H__

// port: the web page's side of the game's cursors (framework/WebCursor.cpp). Nothing here changes what the game does or
// draws; natively the call does nothing.

namespace Sexy
{

class MemoryImage;

// The image the site's cursor set takes for one kind (CURSOR_POINTER to CURSOR_WAIT, framework/SexyAppBase.h), in place of
// the app's mCursorImages[kind]: for a game that keeps its cursors apart and shows each as CURSOR_CUSTOM (Bonnie's
// Bookstore's arrow and hand), so its set can still be read. NULL takes it back; a game that frees the image sets NULL
// first. The cursor the game shows is untouched.
void WebCursorSetKind(int theCursorNum, MemoryImage* theImage);

}

#endif
