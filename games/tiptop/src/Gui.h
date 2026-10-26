#ifndef __TIPTOP_GUI_H__
#define __TIPTOP_GUI_H__

// TipTop's user interface toolkit (TipTop.exe 0x427375-0x42cfae): image registration, bitmap fonts, the
// configuration tokenizer, and a small windowing system of layers (screen areas, nested, some modal) and handlers
// (a procedure and its data, like a window procedure) that the main loop sends its events to (0x429711).
//
// Messages a handler gets (its procedure's first argument):
//   0 create (a, b: the creator's arguments)     1 destroy
//   2 mouse down (x, y)   3 mouse move (x, y)   4 mouse up (x, y)   5 key (DirectInput scan code)
//   6 update   7 draw (a: 1 if the handler must redraw, its layer was marked dirty)
//   8 first handler of its procedure created   9 last one destroyed
//   10 paste   11 tab   12 shift+tab   13 typed character (a)
//   14 its layer is reachable again (a modal layer above it went)   15 a modal layer covers it
//   16 set value (a)   17 get value   18 get length
// Input messages (2-5, 10-13) go only to handlers whose layer is inside the top modal layer.

#include "Lowos.h"

#include <cstdint>

namespace tiptop
{

enum
{
	MSG_CREATE = 0,
	MSG_DESTROY = 1,
	MSG_MOUSEDOWN = 2,
	MSG_MOUSEMOVE = 3,
	MSG_MOUSEUP = 4,
	MSG_KEY = 5,
	MSG_UPDATE = 6,
	MSG_DRAW = 7,
	MSG_CLASSINIT = 8,
	MSG_CLASSFREE = 9,
	MSG_PASTE = 10,
	MSG_TAB = 11,
	MSG_BACKTAB = 12,
	MSG_CHAR = 13,
	MSG_UNCOVERED = 14,
	MSG_COVERED = 15,
	MSG_SETVALUE = 16,
	MSG_GETVALUE = 17,
	MSG_GETLENGTH = 18,
};

struct GuiRect
{
	int16_t x, y, w, h;
};

struct Handler;
struct Layer;

typedef uint32_t (*HandlerProc)(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);

// A bitmap font (0x20c bytes, 0x4285ce): one row of glyphs, separated by columns of the bottom-left pixel's color.
// The original has 128 glyph entries and reads past them for characters from 0x80 (the image id, then the heap);
// the port's 256 leave those empty, so they draw as spaces.
struct Font
{
	lowos::Surface* mSurface;  // +0x00
	uint8_t mSpaceWidth;       // +0x04 the advance of a character without a glyph
	uint8_t mSpacing;          // +0x05 between glyphs
	struct Glyph
	{
		uint16_t mX;           // +0x06 + c * 4
		uint8_t mWidth;        // +0x08 + c * 4
		uint8_t mPad;
	} mGlyphs[256];
	int32_t mImageId;          // +0x208
};

// What an animation shows (0x42cbe4, 12 bytes in the original)
struct AnimationParams
{
	lowos::Surface* mImage;
	uint16_t mDelay;           // ms before it appears
	uint16_t mRise;            // pixels it rises
	uint16_t mFade;            // ms of fading in plus fading out
	uint16_t mHold;            // ms between them
};

// A layer (0x30 bytes, 0x428fd3): an area of the screen with its scrollbars, in a tree; modal layers form a stack.
struct Layer
{
	GuiRect mRect;             // +0x00 (w 0: the slot is free)
	uint16_t mContentWidth;    // +0x08
	uint16_t mContentHeight;   // +0x0a
	Handler* mVScroll;         // +0x0c
	Handler* mHScroll;         // +0x10
	Layer* mChild;             // +0x14
	Layer* mNext;              // +0x18
	Layer* mPrev;              // +0x1c
	Layer* mParent;            // +0x20
	uint8_t mDirty;            // +0x24
	Layer* mModalBelow;        // +0x28
	Layer* mModalAbove;        // +0x2c
};

// A handler (0x30 bytes, 0x4294f7) and its procedure's data
struct Handler
{
	HandlerProc mProc;         // +0x00 (0: the slot is free)
	Layer* mLayer;             // +0x04
	GuiRect mRect;             // +0x08
	uint8_t mPass;             // +0x10 the draw pass, 0..5 (later passes draw over earlier ones)
	int32_t mPending;          // +0x14 events of the dispatch in progress not delivered yet
	uint8_t mVisited;          // +0x18 the draw dispatch's nesting bits
	union                      // +0x1c
	{
		struct  // image buttons (0x42b3eb, 0x42b6b4): two frames, normal over hovered
		{
			lowos::Surface* mImage;   // +0x1c
			uint32_t mCommand;        // +0x20 | 0x80000000 Enter presses it, | 0x40000000 Escape presses it
			int32_t mImageId;         // +0x24
			uint8_t mState;           // +0x28 1 hovered, 2 hovered at the last draw
		} button;
		struct  // check boxes (0x42bd93): four frames, off/on over normal/hovered
		{
			lowos::Surface* mImage;   // +0x1c
			uint8_t mState;           // +0x20 1 checked, 2 hovered
			int32_t mImageId;         // +0x24
			uint32_t mCommand;        // +0x28
		} check;
		struct  // sliders (0x42bfdd): the knob is image 0
		{
			uint8_t mState;           // +0x1c 1 dragged
			uint32_t mCommand;        // +0x20
			int32_t mPos;             // +0x24 the knob's x in the slider
			int32_t mGrab;            // +0x28 the mouse's x on the knob, from the slider's left
		} slider;
		struct  // static images (0x42c36b)
		{
			lowos::Surface* mImage;   // +0x1c
			int32_t mImageId;         // +0x20
			uint8_t mOwned;           // +0x24 registered by the handler
		} image;
		struct  // text labels (0x42bc76) and text areas (0x42ba49)
		{
			const char* mText;        // +0x1c
			Font* mFont;              // +0x20
			uint8_t mAlign;           // +0x24 label: 1 centered horizontally, 2 vertically
		} text;
		struct  // text input (0x42c482)
		{
			char* mBuffer;            // +0x1c
			uint16_t mCommand;        // +0x20
			uint16_t mSize;           // +0x22 the buffer's size
			Font* mFont;              // +0x24
			uint32_t mBlinkTime;      // +0x28
			uint8_t mState;           // +0x2c 1 focused, 2 Enter typed, 4 tab, 8 shift+tab, 0x10 lost focus to a modal
		} input;
		struct  // animations (0x42cbe4): a strip of frames that fades in, holds and fades out
		{
			lowos::Surface* mImage;   // +0x1c
			uint16_t mDelay;          // +0x20 before it appears
			uint16_t mRise;           // +0x22 how far it rises while fading
			uint16_t mFade;           // +0x24 fade in plus fade out
			uint16_t mHold;           // +0x26
			uint32_t mStart;          // +0x28
			uint32_t mFlags;          // +0x2c 0x80000000 additive, low 16 bits: the command sent when it ends
		} anim;
		struct  // reference counters (0x42cf34)
		{
			char* mCounter;           // +0x1c
			Handler** mBackRef;       // +0x20
		} counter;
		struct  // resources kept loaded (0x42cfae)
		{
			int32_t mGroup;           // +0x1c
			int32_t mId;              // +0x20
			uint8_t mLoaded;          // +0x24
		} res;
		struct  // scrollbars (0x429fd5)
		{
			uint8_t mState;           // +0x1c low 3 bits the part pressed, 8 pressed, 0x10 redraw
			int16_t mGrabY;           // +0x1e
			uint16_t mGrabPos;        // +0x20
			uint16_t mMouse;          // +0x22
			uint16_t mContent;        // +0x24
			uint16_t mPos;            // +0x26
			uint16_t mRepeat;         // +0x28
		} scroll;
		uint32_t mWords[5];
		uintptr_t mSlots[5];      // the game's own procedures (App.cpp): param_4[7..11] of the original
	} u;
};

// --- images (0x427375-0x4274d9) ---
// Loads image theId of group 0 and registers it as a surface; *theSurface is cleared on failure. Returns 1 on failure.
char LoadSurface(int theId, uint8_t theFlags, lowos::Surface** theSurface);  // 0x427375
void FreeSurface(lowos::Surface* theSurface);                                 // 0x427451
char ReloadSurfaces();                                                         // 0x4274d9

// --- the configuration tokenizer (0x4280b1-0x428423) ---
int IsSeparator(char c);                                                       // 0x4280b1
int SkipToToken(const char** theText, char stopAtLineEnd);                     // 0x428186
int SkipPast(const char** theText, char theChar);                              // 0x42828e
char TokenChar(int theIndex);                                                  // 0x42838f
const char* Token();                                                           // 0x42839d
void ReadToken(const char** theText);                                          // 0x4283a7
int32_t TokenInt();                                                            // 0x428413
double TokenFloat();                                                           // 0x428423

// --- fonts and text (0x4285ce-0x42881c) ---
Font* LoadFont(int theImageId, const char* theChars, uint8_t theSpaceWidth, uint8_t theSpacing,
	uint8_t theFlags);  // 0x4285ce
void FreeFont(Font* theFont);                                                  // 0x4287fb
// Draws (or measures, flag 2) a line of text; returns the rest of the text after a line break or where wrapping
// stopped, 0 at its end. Flags: 1 wrap at a word, 2 measure only, 4 center in theClip horizontally, 8 vertically,
// 0x10 translucent with theLevel, 0x20 right-aligned at theX (callers pass 0x40 too, which it ignores). theDest 0
// draws to the screen, else into a buffer of the display's format.
const char* DrawText(Font* theFont, uint8_t* theDest, uint32_t theDestPitch, const char* theText,
	const GuiRect* theClip, GuiRect* theExtent, int16_t theX, int16_t theY, uint8_t theLevel,
	uint8_t theFlags);  // 0x42881c

// The glyph blits (0x40a792-0x40aaf9): theLevel 0xff opaque
char ClipRect(int16_t x, int16_t y, int16_t w, int16_t h, const GuiRect* theBounds, GuiRect* theOut);  // 0x40a792
void BlitGlyph(lowos::Surface* theSurface, int16_t x, int16_t y, const GuiRect* theSrc, const GuiRect* theClip,
	uint8_t theLevel);  // 0x40a8e0
void GlyphToBuffer(lowos::Surface* theSurface, uint8_t* theDest, uint32_t theDestPitch, int16_t x, int16_t y,
	const GuiRect* theSrc, const GuiRect* theClip, uint8_t theLevel);  // 0x40aaf9

// --- rectangles ---
void InflateDirty(GuiRect* theRect);                                           // 0x428d6d
int PointInRect(uint16_t x, uint16_t y, const GuiRect* theRect);               // 0x428e40
uint16_t LayerScrollY(Layer* theLayer);                                        // 0x428e98

// --- layers and handlers (0x428eb5-0x429c75) ---
void GuiInit(uint16_t theLayers, uint16_t theHandlers, lowos::Surface* theSkin);  // 0x428eb5
void GuiShutdown();                                                            // 0x428fa3
Layer* NewLayer(uint16_t theContentWidth, uint16_t theContentHeight, const GuiRect* theRect);  // 0x428fd3
void PushModal(Layer* theLayer);                                               // 0x42907f
// port (test hooks): every layer in use draws all its handlers again in the next frame
void MarkAllLayersDirty();
char InTopModal(Layer* theLayer);                                              // 0x42917f
char IsInside(Layer* theLayer, Layer* theAncestor);                            // 0x4291bf
void FreeLayer(Layer* theLayer);                                               // 0x4291f9
void AddChildLayer(Layer* theParent, Layer* theChild);                         // 0x42946d
void FreeAllLayers();                                                          // 0x4294ac
Handler* NewHandler(HandlerProc theProc, Layer* theLayer, const GuiRect* theRect, uintptr_t theA,
	uintptr_t theB);  // 0x4294f7
void FreeHandler(Handler* theHandler);                                         // 0x429611
Handler* NextHandler(Handler* theHandler);                                     // 0x429692
Handler* PrevHandler(Handler* theHandler);                                     // 0x4296d1
void Dispatch(uint32_t theMsg, uintptr_t theA, uintptr_t theB);                // 0x429711
uint32_t Send(Handler* theHandler, uint32_t theMsg, uintptr_t theA, uintptr_t theB);  // 0x429c75
void SetLayerContent(Layer* theLayer, uint16_t theWidth, uint16_t theHeight);  // 0x429ca4

// --- the toolkit's handler procedures ---
uint32_t ScrollBarProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);    // 0x429fd5
uint32_t ButtonProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);       // 0x42b3eb
uint32_t ShapedButtonProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler); // 0x42b6b4
uint32_t TextAreaProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);     // 0x42ba49
uint32_t LabelProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);        // 0x42bc76
uint32_t CheckBoxProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);     // 0x42bd93
uint32_t SliderProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);       // 0x42bfdd
uint32_t ImageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);        // 0x42c36b
uint32_t TextInputProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);    // 0x42c482
uint32_t AnimationProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);    // 0x42cbe4
uint32_t CounterProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);      // 0x42cf34
uint32_t ResourceProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);     // 0x42cfae

} // namespace tiptop

#endif
