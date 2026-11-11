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
	if (theInput->caret == theInput->length)
		return theInput->caretWidth;
	return Font_CharAdvance(theInput->font, theInput->text[theInput->caret]);
}

// 0x42c6b0: how far the caret is from the text's start
int CaretX(TextInput* theInput)
{
	int aX = 0;
	int i = 0;
	if (0 < theInput->caret)
	{
		do
		{
			int anAdvance = Font_CharAdvance(theInput->font, theInput->text[i]);
			i = i + 1;
			aX = aX + anAdvance;
		} while (i < theInput->caret);
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
	if (theInput->active == 0)
		return;
	if (theInput->fillColour[3] != 0)
		Draw_FillBlend(theDst, theDPitch, theFmt, theInput->box[0], theInput->box[1], theInput->box[2], theInput->box[3],
			theInput->fillColour[0], theInput->fillColour[1], theInput->fillColour[2], theInput->fillColour[3]);
	if (theInput->frameColour[3] != 0)
	{
		const uint8_t* c = theInput->frameColour;
		const int* f = theInput->frame;
		Draw_LineOn(theDst, theDPitch, theFmt, f[0], f[1], f[2] - 1, f[1], c[0], c[1], c[2], c[3]);
		Draw_LineOn(theDst, theDPitch, theFmt, f[2], f[1], f[2], f[3] - 1, c[0], c[1], c[2], c[3]);
		Draw_LineOn(theDst, theDPitch, theFmt, f[2], f[3], f[0] + 1, f[3], c[0], c[1], c[2], c[3]);
		Draw_LineOn(theDst, theDPitch, theFmt, f[0], f[3], f[0], f[1] + 1, c[0], c[1], c[2], c[3]);
	}
	int aLeft = theInput->textRect[0];
	int aScroll = theInput->scroll;
	int aHeight = Font_Height(theInput->font);
	int aBottom = theInput->textRect[3];
	Font_SetTarget(theInput->font, theDst, theDPitch, theFmt, 0, 0);
	Font_ResetStyle(theInput->font);
	Font_SetEscapes(theInput->font, 0);
	Font_SetOutline(theInput->font, theInput->outline);
	Font_SetClip(theInput->font, theInput->inner[0], theInput->inner[1], theInput->inner[2], theInput->inner[3]);
	Font_UseColour(theInput->font, theInput->colourCode);
	int aRect[4];
	Font_DrawOutlined(theInput->font, aRect, aLeft - aScroll, aBottom - aHeight, theInput->text);
}

// 0x42cd70: scrolls the text so the caret shows
void KeepCaretInView(TextInput* theInput)
{
	int aWidth;
	if (theInput->caret == theInput->length)
		aWidth = theInput->caretWidth;
	else
		aWidth = Font_CharAdvance(theInput->font, theInput->text[theInput->caret]);
	int aX = CaretX(theInput);
	int aShown = (theInput->textRect[2] - theInput->textRect[0]) + 1;
	if (aX < theInput->scroll)
		theInput->scroll = aX;
	if (theInput->scroll + aShown < aWidth + aX)
		theInput->scroll = (aWidth - aShown) + aX;
}

} // namespace

TextInput::TextInput()
{
	text = nullptr;
	TextInput_Clear(this);
}

TextInput::~TextInput()
{
	if (text != nullptr)
	{
		free(text);
		text = nullptr;
	}
	TextInput_Clear(this);
}

void TextInput_Clear(TextInput* theInput)
{
	theInput->font = nullptr;
	theInput->x = 0;
	theInput->y = 0;
	theInput->w = 0;
	theInput->h = 0;
	theInput->box[0] = 0;
	theInput->box[1] = 0;
	theInput->box[2] = 0;
	theInput->box[3] = 0;
	theInput->active = 0;
	theInput->frame[0] = 0;
	theInput->frame[1] = 0;
	theInput->frame[2] = 0;
	theInput->frame[3] = 0;
	theInput->inner[0] = 0;
	theInput->inner[1] = 0;
	theInput->inner[2] = 0;
	theInput->inner[3] = 0;
	theInput->padX = 0;
	theInput->padY = 0;
	theInput->textRect[0] = 0;
	theInput->textRect[1] = 0;
	theInput->textRect[2] = 0;
	theInput->textRect[3] = 0;
	SetColour(theInput->frameColour, 0, 0, 0, 0);
	SetColour(theInput->fillColour, 0, 0, 0, 0);
	theInput->colourCode = 0;
	theInput->focused = 0;
	SetColour(theInput->caretColour, 0, 0, 0, 0);
	theInput->blinkTime = 0;
	theInput->blinkClock = 0;
	theInput->caretOn = 0;
	theInput->caret = 0;
	theInput->caretWidth = 0;
	theInput->caretHeight = 0;
	theInput->caretDrop = 0;
	theInput->onEnter = nullptr;
	theInput->onKey = nullptr;
	theInput->onFocus = nullptr;
	theInput->entered = 0;
	theInput->typed = 0;
	theInput->clickedIn = 0;
	theInput->maxLength = 0;
	theInput->length = 0;
	theInput->text = nullptr;
	theInput->scroll = 0;
	theInput->pressed = 0;
	theInput->outline = nullptr;
	memset(theInput->blocked, 0, sizeof theInput->blocked);
	theInput->noFocus = 0;
	theInput->caseMode = 0;
}

// 0x42c3b0
void TextInput_Init(TextInput* theInput, Font* theFont, int theMaxLength, char theColourCode, int theLeft, int theTop, int theRight,
	int theBottom)
{
	if (theInput->text != nullptr)
	{
		free(theInput->text);
		theInput->text = nullptr;
	}
	TextInput_Clear(theInput);
	theInput->font = theFont;
	theInput->colourCode = theColourCode;
	theInput->w = (theRight - theLeft) + 1;
	theInput->box[0] = theLeft;
	theInput->box[2] = theRight;
	theInput->x = theLeft;
	theInput->box[1] = theTop;
	theInput->box[3] = theBottom;
	theInput->y = theTop;
	theInput->frame[0] = theInput->box[0];
	theInput->frame[1] = theInput->box[1];
	theInput->active = 1;
	theInput->frame[2] = theInput->box[2];
	theInput->frame[3] = theBottom;
	theInput->inner[0] = theInput->frame[0] + 1;
	theInput->inner[1] = theInput->frame[1] + 1;
	theInput->inner[2] = theInput->frame[2] - 1;
	theInput->h = (theBottom - theTop) + 1;
	theInput->inner[3] = theInput->frame[3] - 1;
	theInput->textRect[0] = theInput->frame[0] + 4;
	theInput->padX = 4;
	theInput->padY = 4;
	theInput->textRect[1] = theInput->frame[1] + 4;
	theInput->textRect[2] = theInput->frame[2] - 4;
	theInput->textRect[3] = theInput->frame[3] - 4;
	SetColour(theInput->frameColour, 0xff, 0xff, 0xff, 0xff);
	SetColour(theInput->fillColour, 0, 0, 0, 0xff);
	theInput->focused = 1;
	SetColour(theInput->caretColour, 0xff, 0xff, 0, 200);
	theInput->blinkTime = 0.4;
	theInput->blinkClock = 0;
	theInput->caretOn = 0;
	theInput->caret = 0;
	theInput->caretHeight = 2;
	theInput->caretDrop = 0;
	char aChar;
	if (Font_CharAdvance(theInput->font, 'A') < 1)
		aChar = Font_CharAdvance(theInput->font, 'a') < 1 ? ' ' : 'a';
	else
		aChar = 'A';
	theInput->caretWidth = Font_CharAdvance(theInput->font, aChar);
	theInput->maxLength = theMaxLength;
	theInput->onEnter = nullptr;
	theInput->onKey = nullptr;
	theInput->onFocus = nullptr;
	theInput->entered = 0;
	theInput->typed = 0;
	theInput->clickedIn = 0;
	theInput->length = 0;
	theInput->text = (char*)malloc(theMaxLength + 1);
	memset(theInput->text, 0, theInput->maxLength + 1);
	theInput->scroll = 0;
	theInput->pressed = 0;
	// The original clears 0x100 entries here, the last of them noFocus
	memset(theInput->blocked, 0, sizeof theInput->blocked);
	theInput->noFocus = 0;
	Key_Clear();
}

void TextInput_SetFillColour(TextInput* theInput, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	theInput->fillColour[0] = r;
	theInput->fillColour[1] = g;
	theInput->fillColour[2] = b;
	theInput->fillColour[3] = a;
}

void TextInput_SetOutline(TextInput* theInput, char theCode)
{
	theInput->outline = theInput->font->colours[(int)theCode];
}

void TextInput_SetOnEnter(TextInput* theInput, void (*theFunc)(const char*))
{
	theInput->onEnter = theFunc;
}

void TextInput_SetFocus(TextInput* theInput, int theFocused)
{
	theInput->focused = theFocused;
	if (theInput->onFocus != nullptr)
		theInput->onFocus(theFocused);
}

char* TextInput_GetText(TextInput* theInput)
{
	return theInput->text;
}

void TextInput_SetCaseMode(TextInput* theInput, int theMode)
{
	theInput->caseMode = theMode;
}

void TextInput_Draw(TextInput* theInput, uint8_t* theDst, int theDPitch, const PixelFormat* theFmt, Dirty* theDirty)
{
	if (theInput->active == 0)
		return;
	DrawText(theInput, theDst, theDPitch, theFmt);
	Dirty_AddClippedShown(theDirty, theInput->x, theInput->y, theInput->w, theInput->h, theDirty->srcRect);
	if (theInput->focused == 1 && theInput->caretOn == 1)
	{
		int aWidth = CaretWidth(theInput);
		int aX = CaretX(theInput);
		int aLeft = theInput->textRect[0];
		int aScroll = theInput->scroll;
		int aBottom = theInput->textRect[3] - theInput->caretDrop;
		int aTop = aBottom - theInput->caretHeight;
		const uint8_t* c = theInput->caretColour;
		Draw_FillBlendClipped(theDst, theDPitch, theFmt, theInput->inner, (aLeft - aScroll) + aX, aTop,
			(aLeft - aScroll) + aX + aWidth - 1, aBottom, c[0], c[1], c[2], c[3]);
		Dirty_AddClippedShown(theDirty, (aLeft - aScroll) + aX, aTop, aWidth, (aBottom - aTop) + 1, theDirty->srcRect);
	}
}

void TextInput_SetText(TextInput* theInput, const char* theText)
{
	memset(theInput->text, 0, theInput->maxLength + 1);
	theInput->length = 0;
	theInput->caret = 0;
	strcpy(theInput->text, theText);
	int aLength = (int)strlen(theInput->text);
	theInput->length = aLength;
	theInput->caret = aLength;
}

void TextInput_Update(TextInput* theInput, double theSeconds)
{
	if (theInput->active == 0)
		return;
	int aMouseX, aMouseY;
	Mouse_Update(&aMouseX, &aMouseY);
	theInput->entered = 0;
	theInput->typed = 0;
	theInput->clickedIn = 0;
	if (theInput->noFocus == 0 && theInput->box[0] <= aMouseX && aMouseX <= theInput->box[2] &&
		theInput->box[1] <= aMouseY && aMouseY <= theInput->box[3])
	{
		if (theInput->focused != 1)
		{
			if (Mouse_State(1) == 1)
				theInput->pressed = 1;
			else
			{
				if (theInput->pressed == 1)
				{
					theInput->focused = 1;
					if (theInput->onFocus != nullptr)
						theInput->onFocus(1);
					theInput->clickedIn = 1;
					Key_Clear();
					theInput->blinkClock = 0;
					theInput->caretOn = 1;
				}
				theInput->pressed = 0;
			}
			if (theInput->focused != 1)
				goto blink;
		}
	}
	else
	{
		theInput->pressed = 0;
		Mouse_State(1);
		if (theInput->noFocus == 1)
			theInput->focused = 0;
		if (theInput->focused != 1)
			goto blink;
	}
	{
		int aKey = Key_Pop();
		uint8_t aCode = (uint8_t)aKey;
		aKey = aKey & 0xff;
		if (aCode == 0)
			goto blink;
		if (theInput->onKey != nullptr)
			theInput->onKey(aKey);
		theInput->blinkClock = 0;
		theInput->typed = 1;
		theInput->caretOn = 1;
		if (IsTypable(aCode) == 1)
		{
			uint8_t aChar = VkToChar(aCode);
			int aMode = theInput->caseMode;
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
			if (theInput->blocked[aChar] == 0 && 0 < Font_CharWidth(theInput->font, (char)aChar) &&
				theInput->length < theInput->maxLength)
			{
				int aCaret = theInput->caret;
				if (aCaret == theInput->length)
				{
					theInput->text[aCaret] = (char)aChar;
					theInput->length = theInput->length + 1;
					theInput->caret = theInput->caret + 1;
				}
				else
				{
					int i = theInput->maxLength - 1;
					if (aCaret < i)
					{
						do
						{
							theInput->text[i] = theInput->text[i - 1];
							i = i - 1;
						} while (theInput->caret < i);
					}
					theInput->text[theInput->caret] = (char)aChar;
					theInput->length = theInput->length + 1;
					theInput->caret = theInput->caret + 1;
				}
			}
		}
		else if (aCode == 0x0d)
		{
			if (theInput->onEnter != nullptr)
				theInput->onEnter(theInput->text);
			theInput->entered = 1;
		}
		else if (aCode == 0x08)
		{
			if (0 < theInput->caret && 0 < theInput->length)
			{
				int aScroll;
				if (theInput->scroll - Font_CharAdvance(theInput->font, theInput->text[theInput->caret - 1]) < 0)
					aScroll = 0;
				else
					aScroll = theInput->scroll - Font_CharAdvance(theInput->font, theInput->text[theInput->caret - 1]);
				int i = theInput->caret - 1;
				theInput->scroll = aScroll;
				if (i < theInput->length)
				{
					do
					{
						theInput->text[i] = theInput->text[i + 1];
						i = i + 1;
					} while (i < theInput->length);
				}
				theInput->text[theInput->length] = 0;
				theInput->caret = theInput->caret - 1;
				theInput->length = theInput->length - 1;
			}
		}
		else if (aCode == 0x2e)
		{
			int i = theInput->caret;
			if (i < theInput->length && 0 < theInput->length)
			{
				do
				{
					theInput->text[i] = theInput->text[i + 1];
					i = i + 1;
				} while (i < theInput->length);
				theInput->text[theInput->length] = 0;
				theInput->length = theInput->length - 1;
			}
		}
		else if (aCode == 0x25)
		{
			if (0 < theInput->caret)
				theInput->caret = theInput->caret - 1;
		}
		else if (aCode == 0x27 && theInput->caret < theInput->length)
			theInput->caret = theInput->caret + 1;
		KeepCaretInView(theInput);
	}
blink:
	theInput->blinkClock = theInput->blinkClock + theSeconds;
	if (theInput->blinkTime < theInput->blinkClock)
	{
		do
		{
			theInput->blinkClock = theInput->blinkClock - theInput->blinkTime;
			theInput->caretOn = theInput->caretOn != 1 ? 1 : 0;
		} while (theInput->blinkTime < theInput->blinkClock);
	}
}

} // namespace r64
