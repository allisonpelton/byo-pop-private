// Retro64's one-line text entry (0x42c260..0x42d1f0); see R64TextInput.h.

#include "R64TextInput.h"
#include "R64Sys.h"

#include <cstdlib>
#include <cstring>

namespace r64
{

namespace
{

// 0x42c260
void SetColour(uint8_t* theColour, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	theColour[0] = r;
	theColour[1] = g;
	theColour[2] = b;
	theColour[3] = a;
}

// 0x42c680: how wide the caret is where it stands (the character's advance; at the end, caretWidth)
int CaretWidth(TextInput* theInput)
{
	if (theInput->mCaret == theInput->mLength)
		return theInput->mCaretWidth;
	return Font_CharAdvance(theInput->mFont, theInput->mText[theInput->mCaret]);
}

// 0x42c6b0: how far the caret is from the text's start
int CaretX(TextInput* theInput)
{
	int aX = 0;
	int i = 0;
	if (0 < theInput->mCaret)
	{
		do
		{
			int anAdvance = Font_CharAdvance(theInput->mFont, theInput->mText[i]);
			i = i + 1;
			aX = aX + anAdvance;
		} while (i < theInput->mCaret);
	}
	return aX;
}

// 0x42c6f0: a virtual-key code's character, unshifted (0 for keys that type none)
uint8_t VkToChar(uint8_t theKey)
{
	if ((uint8_t)(theKey - 0x30) > 9)
	{
		if ((uint8_t)(theKey - 0x41) < 0x1a)
			return theKey + 0x20;
		if (theKey == 0x20)
			return 0x20;
		if (theKey == 0xbc)
			return ',';
		if (theKey == 0xbe)
			return '.';
		if (theKey == 0xbd)
			return '-';
		if (theKey == 0xbb)
			return '=';
		if (theKey == 0xdb)
			return '[';
		if (theKey == 0xdd)
			return ']';
		if (theKey == 0xdc)
			return '\\';
		if (theKey == 0xba)
			return ';';
		if (theKey == 0xde)
			return '\'';
		if (theKey == 0xbf)
			return '/';
		theKey = theKey == 0xc0 ? '`' : 0;
	}
	return theKey;
}

// 0x42c780: the character shift makes of one (on a US keyboard)
uint8_t Shifted(uint8_t theChar)
{
	if ((uint8_t)(theChar - 0x30) < 10)
	{
		if (theChar == '0')
			return ')';
		if (theChar == '1')
			return '!';
		if (theChar == '2')
			return '@';
		if (theChar == '3')
			return '#';
		if (theChar == '4')
			return '$';
		if (theChar == '5')
			return '%';
		if (theChar == '6')
			return '^';
		if (theChar == '7')
			return '&';
		if (theChar == '8')
			return '*';
		if (theChar == '9')
			return '(';
	}
	else
	{
		if ((uint8_t)(theChar - 0x61) < 0x1a)
			return theChar - 0x20;
		if (theChar == ' ')
			return ' ';
		if (theChar == ',')
			return '<';
		if (theChar == '.')
			return '>';
		if (theChar == '-')
			return '_';
		if (theChar == '=')
			return '+';
		if (theChar == '[')
			return '{';
		if (theChar == ']')
			return '}';
		if (theChar == '\\')
			return '|';
		if (theChar == ';')
			return ':';
		if (theChar == '\'')
			return '"';
		if (theChar == '/')
			return '?';
		theChar = theChar == '`' ? '~' : 0;
	}
	return theChar;
}

// 0x42c870: 1 for the keys that type a character
int IsTypable(uint8_t theKey)
{
	if ((theKey < 0x30 || 0x39 < theKey) && (theKey < 0x41 || 0x5a < theKey) && theKey != 0x20 && theKey != 0xbc &&
		theKey != 0xbe && theKey != 0xbd && theKey != 0xbb && theKey != 0xdb && theKey != 0xdd && theKey != 0xdc &&
		theKey != 0xba && theKey != 0xde && theKey != 0xbf && theKey != 0xc0)
		return 0;
	return 1;
}

// 0x42c8d0: the fill, the frame and the text
void DrawText(TextInput* theInput, uint8_t* theDst, int theDPitch, const PixelFormat* theFmt)
{
	if (theInput->mActive == 0)
		return;
	if (theInput->mFillColour[3] != 0)
		Draw_FillBlend(theDst, theDPitch, theFmt, theInput->mBox[0], theInput->mBox[1], theInput->mBox[2], theInput->mBox[3],
			theInput->mFillColour[0], theInput->mFillColour[1], theInput->mFillColour[2], theInput->mFillColour[3]);
	if (theInput->mFrameColour[3] != 0)
	{
		const uint8_t* c = theInput->mFrameColour;
		const int* f = theInput->mFrame;
		Draw_LineOn(theDst, theDPitch, theFmt, f[0], f[1], f[2] - 1, f[1], c[0], c[1], c[2], c[3]);
		Draw_LineOn(theDst, theDPitch, theFmt, f[2], f[1], f[2], f[3] - 1, c[0], c[1], c[2], c[3]);
		Draw_LineOn(theDst, theDPitch, theFmt, f[2], f[3], f[0] + 1, f[3], c[0], c[1], c[2], c[3]);
		Draw_LineOn(theDst, theDPitch, theFmt, f[0], f[3], f[0], f[1] + 1, c[0], c[1], c[2], c[3]);
	}
	int aLeft = theInput->mTextRect[0];
	int aScroll = theInput->mScroll;
	int aHeight = Font_Height(theInput->mFont);
	int aBottom = theInput->mTextRect[3];
	Font_SetTarget(theInput->mFont, theDst, theDPitch, theFmt, 0, 0);
	Font_ResetStyle(theInput->mFont);
	Font_SetEscapes(theInput->mFont, 0);
	Font_SetOutline(theInput->mFont, theInput->mOutline);
	Font_SetClip(theInput->mFont, theInput->mInner[0], theInput->mInner[1], theInput->mInner[2], theInput->mInner[3]);
	Font_UseColour(theInput->mFont, theInput->mColourCode);
	int aRect[4];
	Font_DrawOutlined(theInput->mFont, aRect, aLeft - aScroll, aBottom - aHeight, theInput->mText);
}

// 0x42cd70: scrolls the text so the caret shows
void KeepCaretInView(TextInput* theInput)
{
	int aWidth;
	if (theInput->mCaret == theInput->mLength)
		aWidth = theInput->mCaretWidth;
	else
		aWidth = Font_CharAdvance(theInput->mFont, theInput->mText[theInput->mCaret]);
	int aX = CaretX(theInput);
	int aShown = (theInput->mTextRect[2] - theInput->mTextRect[0]) + 1;
	if (aX < theInput->mScroll)
		theInput->mScroll = aX;
	if (theInput->mScroll + aShown < aWidth + aX)
		theInput->mScroll = (aWidth - aShown) + aX;
}

} // namespace

TextInput::TextInput()
{
	mText = nullptr;
	TextInput_Clear(this);
}

TextInput::~TextInput()
{
	if (mText != nullptr)
	{
		free(mText);
		mText = nullptr;
	}
	TextInput_Clear(this);
}

void TextInput_Clear(TextInput* theInput)
{
	theInput->mFont = nullptr;
	theInput->mX = 0;
	theInput->mY = 0;
	theInput->mW = 0;
	theInput->mH = 0;
	theInput->mBox[0] = 0;
	theInput->mBox[1] = 0;
	theInput->mBox[2] = 0;
	theInput->mBox[3] = 0;
	theInput->mActive = 0;
	theInput->mFrame[0] = 0;
	theInput->mFrame[1] = 0;
	theInput->mFrame[2] = 0;
	theInput->mFrame[3] = 0;
	theInput->mInner[0] = 0;
	theInput->mInner[1] = 0;
	theInput->mInner[2] = 0;
	theInput->mInner[3] = 0;
	theInput->mPadX = 0;
	theInput->mPadY = 0;
	theInput->mTextRect[0] = 0;
	theInput->mTextRect[1] = 0;
	theInput->mTextRect[2] = 0;
	theInput->mTextRect[3] = 0;
	SetColour(theInput->mFrameColour, 0, 0, 0, 0);
	SetColour(theInput->mFillColour, 0, 0, 0, 0);
	theInput->mColourCode = 0;
	theInput->mFocused = 0;
	SetColour(theInput->mCaretColour, 0, 0, 0, 0);
	theInput->mBlinkTime = 0;
	theInput->mBlinkClock = 0;
	theInput->mCaretOn = 0;
	theInput->mCaret = 0;
	theInput->mCaretWidth = 0;
	theInput->mCaretHeight = 0;
	theInput->mCaretDrop = 0;
	theInput->mOnEnter = nullptr;
	theInput->mOnKey = nullptr;
	theInput->mOnFocus = nullptr;
	theInput->mEntered = 0;
	theInput->mTyped = 0;
	theInput->mClickedIn = 0;
	theInput->mMaxLength = 0;
	theInput->mLength = 0;
	theInput->mText = nullptr;
	theInput->mScroll = 0;
	theInput->mPressed = 0;
	theInput->mOutline = nullptr;
	memset(theInput->mBlocked, 0, sizeof theInput->mBlocked);
	theInput->mNoFocus = 0;
	theInput->mCaseMode = 0;
}

// 0x42c3b0
void TextInput_Init(TextInput* theInput, Font* theFont, int theMaxLength, char theColourCode, int theLeft, int theTop, int theRight,
	int theBottom)
{
	if (theInput->mText != nullptr)
	{
		free(theInput->mText);
		theInput->mText = nullptr;
	}
	TextInput_Clear(theInput);
	theInput->mFont = theFont;
	theInput->mColourCode = theColourCode;
	theInput->mW = (theRight - theLeft) + 1;
	theInput->mBox[0] = theLeft;
	theInput->mBox[2] = theRight;
	theInput->mX = theLeft;
	theInput->mBox[1] = theTop;
	theInput->mBox[3] = theBottom;
	theInput->mY = theTop;
	theInput->mFrame[0] = theInput->mBox[0];
	theInput->mFrame[1] = theInput->mBox[1];
	theInput->mActive = 1;
	theInput->mFrame[2] = theInput->mBox[2];
	theInput->mFrame[3] = theBottom;
	theInput->mInner[0] = theInput->mFrame[0] + 1;
	theInput->mInner[1] = theInput->mFrame[1] + 1;
	theInput->mInner[2] = theInput->mFrame[2] - 1;
	theInput->mH = (theBottom - theTop) + 1;
	theInput->mInner[3] = theInput->mFrame[3] - 1;
	theInput->mTextRect[0] = theInput->mFrame[0] + 4;
	theInput->mPadX = 4;
	theInput->mPadY = 4;
	theInput->mTextRect[1] = theInput->mFrame[1] + 4;
	theInput->mTextRect[2] = theInput->mFrame[2] - 4;
	theInput->mTextRect[3] = theInput->mFrame[3] - 4;
	SetColour(theInput->mFrameColour, 0xff, 0xff, 0xff, 0xff);
	SetColour(theInput->mFillColour, 0, 0, 0, 0xff);
	theInput->mFocused = 1;
	SetColour(theInput->mCaretColour, 0xff, 0xff, 0, 200);
	theInput->mBlinkTime = 0.4;
	theInput->mBlinkClock = 0;
	theInput->mCaretOn = 0;
	theInput->mCaret = 0;
	theInput->mCaretHeight = 2;
	theInput->mCaretDrop = 0;
	char aChar;
	if (Font_CharAdvance(theInput->mFont, 'A') < 1)
		aChar = Font_CharAdvance(theInput->mFont, 'a') < 1 ? ' ' : 'a';
	else
		aChar = 'A';
	theInput->mCaretWidth = Font_CharAdvance(theInput->mFont, aChar);
	theInput->mMaxLength = theMaxLength;
	theInput->mOnEnter = nullptr;
	theInput->mOnKey = nullptr;
	theInput->mOnFocus = nullptr;
	theInput->mEntered = 0;
	theInput->mTyped = 0;
	theInput->mClickedIn = 0;
	theInput->mLength = 0;
	theInput->mText = (char*)malloc(theMaxLength + 1);
	memset(theInput->mText, 0, theInput->mMaxLength + 1);
	theInput->mScroll = 0;
	theInput->mPressed = 0;
	// The original clears 0x100 entries here, the last of them noFocus
	memset(theInput->mBlocked, 0, sizeof theInput->mBlocked);
	theInput->mNoFocus = 0;
	Key_Clear();
}

void TextInput_SetFillColour(TextInput* theInput, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	theInput->mFillColour[0] = r;
	theInput->mFillColour[1] = g;
	theInput->mFillColour[2] = b;
	theInput->mFillColour[3] = a;
}

void TextInput_SetOutline(TextInput* theInput, char theCode)
{
	theInput->mOutline = theInput->mFont->mColours[(int)theCode];
}

void TextInput_SetOnEnter(TextInput* theInput, void (*theFunc)(const char*))
{
	theInput->mOnEnter = theFunc;
}

void TextInput_SetFocus(TextInput* theInput, int theFocused)
{
	theInput->mFocused = theFocused;
	if (theInput->mOnFocus != nullptr)
		theInput->mOnFocus(theFocused);
}

char* TextInput_GetText(TextInput* theInput)
{
	return theInput->mText;
}

void TextInput_SetCaseMode(TextInput* theInput, int theMode)
{
	theInput->mCaseMode = theMode;
}

void TextInput_Draw(TextInput* theInput, uint8_t* theDst, int theDPitch, const PixelFormat* theFmt, Dirty* theDirty)
{
	if (theInput->mActive == 0)
		return;
	DrawText(theInput, theDst, theDPitch, theFmt);
	Dirty_AddClippedShown(theDirty, theInput->mX, theInput->mY, theInput->mW, theInput->mH, theDirty->mSrcRect);
	if (theInput->mFocused == 1 && theInput->mCaretOn == 1)
	{
		int aWidth = CaretWidth(theInput);
		int aX = CaretX(theInput);
		int aLeft = theInput->mTextRect[0];
		int aScroll = theInput->mScroll;
		int aBottom = theInput->mTextRect[3] - theInput->mCaretDrop;
		int aTop = aBottom - theInput->mCaretHeight;
		const uint8_t* c = theInput->mCaretColour;
		Draw_FillBlendClipped(theDst, theDPitch, theFmt, theInput->mInner, (aLeft - aScroll) + aX, aTop,
			(aLeft - aScroll) + aX + aWidth - 1, aBottom, c[0], c[1], c[2], c[3]);
		Dirty_AddClippedShown(theDirty, (aLeft - aScroll) + aX, aTop, aWidth, (aBottom - aTop) + 1, theDirty->mSrcRect);
	}
}

void TextInput_SetText(TextInput* theInput, const char* theText)
{
	memset(theInput->mText, 0, theInput->mMaxLength + 1);
	theInput->mLength = 0;
	theInput->mCaret = 0;
	strcpy(theInput->mText, theText);
	int aLength = (int)strlen(theInput->mText);
	theInput->mLength = aLength;
	theInput->mCaret = aLength;
}

void TextInput_Update(TextInput* theInput, double theSeconds)
{
	if (theInput->mActive == 0)
		return;
	int aMouseX, aMouseY;
	Mouse_Update(&aMouseX, &aMouseY);
	theInput->mEntered = 0;
	theInput->mTyped = 0;
	theInput->mClickedIn = 0;
	if (theInput->mNoFocus == 0 && theInput->mBox[0] <= aMouseX && aMouseX <= theInput->mBox[2] &&
		theInput->mBox[1] <= aMouseY && aMouseY <= theInput->mBox[3])
	{
		if (theInput->mFocused != 1)
		{
			if (Mouse_State(1) == 1)
				theInput->mPressed = 1;
			else
			{
				if (theInput->mPressed == 1)
				{
					theInput->mFocused = 1;
					if (theInput->mOnFocus != nullptr)
						theInput->mOnFocus(1);
					theInput->mClickedIn = 1;
					Key_Clear();
					theInput->mBlinkClock = 0;
					theInput->mCaretOn = 1;
				}
				theInput->mPressed = 0;
			}
			if (theInput->mFocused != 1)
				goto blink;
		}
	}
	else
	{
		theInput->mPressed = 0;
		Mouse_State(1);
		if (theInput->mNoFocus == 1)
			theInput->mFocused = 0;
		if (theInput->mFocused != 1)
			goto blink;
	}
	{
		int aKey = Key_Pop();
		uint8_t aCode = (uint8_t)aKey;
		aKey = aKey & 0xff;
		if (aCode == 0)
			goto blink;
		if (theInput->mOnKey != nullptr)
			theInput->mOnKey(aKey);
		theInput->mBlinkClock = 0;
		theInput->mTyped = 1;
		theInput->mCaretOn = 1;
		if (IsTypable(aCode) == 1)
		{
			uint8_t aChar = VkToChar(aCode);
			int aMode = theInput->mCaseMode;
			if (aMode == 0)
			{
				if (Key_Down(0x10) != 0)
					aChar = Shifted(aChar);
			}
			else if (aMode == 1)
			{
				if ((uint8_t)(aChar - 0x61) <= 0x19 || Key_Down(0x10) != 0)
					aChar = Shifted(aChar);
			}
			else if (aMode == 2 && (aChar < 0x61 || 0x7a < aChar))
			{
				if (Key_Down(0x10) != 0)
					aChar = Shifted(aChar);
			}
			if (theInput->mBlocked[aChar] == 0 && 0 < Font_CharWidth(theInput->mFont, (char)aChar) &&
				theInput->mLength < theInput->mMaxLength)
			{
				int aCaret = theInput->mCaret;
				if (aCaret == theInput->mLength)
				{
					theInput->mText[aCaret] = (char)aChar;
					theInput->mLength = theInput->mLength + 1;
					theInput->mCaret = theInput->mCaret + 1;
				}
				else
				{
					int i = theInput->mMaxLength - 1;
					if (aCaret < i)
					{
						do
						{
							theInput->mText[i] = theInput->mText[i - 1];
							i = i - 1;
						} while (theInput->mCaret < i);
					}
					theInput->mText[theInput->mCaret] = (char)aChar;
					theInput->mLength = theInput->mLength + 1;
					theInput->mCaret = theInput->mCaret + 1;
				}
			}
		}
		else if (aCode == 0x0d)
		{
			if (theInput->mOnEnter != nullptr)
				theInput->mOnEnter(theInput->mText);
			theInput->mEntered = 1;
		}
		else if (aCode == 0x08)
		{
			if (0 < theInput->mCaret && 0 < theInput->mLength)
			{
				int aScroll;
				if (theInput->mScroll - Font_CharAdvance(theInput->mFont, theInput->mText[theInput->mCaret - 1]) < 0)
					aScroll = 0;
				else
					aScroll = theInput->mScroll - Font_CharAdvance(theInput->mFont, theInput->mText[theInput->mCaret - 1]);
				int i = theInput->mCaret - 1;
				theInput->mScroll = aScroll;
				if (i < theInput->mLength)
				{
					do
					{
						theInput->mText[i] = theInput->mText[i + 1];
						i = i + 1;
					} while (i < theInput->mLength);
				}
				theInput->mText[theInput->mLength] = 0;
				theInput->mCaret = theInput->mCaret - 1;
				theInput->mLength = theInput->mLength - 1;
			}
		}
		else if (aCode == 0x2e)
		{
			int i = theInput->mCaret;
			if (i < theInput->mLength && 0 < theInput->mLength)
			{
				do
				{
					theInput->mText[i] = theInput->mText[i + 1];
					i = i + 1;
				} while (i < theInput->mLength);
				theInput->mText[theInput->mLength] = 0;
				theInput->mLength = theInput->mLength - 1;
			}
		}
		else if (aCode == 0x25)
		{
			if (0 < theInput->mCaret)
				theInput->mCaret = theInput->mCaret - 1;
		}
		else if (aCode == 0x27 && theInput->mCaret < theInput->mLength)
			theInput->mCaret = theInput->mCaret + 1;
		KeepCaretInView(theInput);
	}
blink:
	theInput->mBlinkClock = theInput->mBlinkClock + theSeconds;
	if (theInput->mBlinkTime < theInput->mBlinkClock)
	{
		do
		{
			theInput->mBlinkClock = theInput->mBlinkClock - theInput->mBlinkTime;
			theInput->mCaretOn = theInput->mCaretOn != 1 ? 1 : 0;
		} while (theInput->mBlinkTime < theInput->mBlinkClock);
	}
}

} // namespace r64
