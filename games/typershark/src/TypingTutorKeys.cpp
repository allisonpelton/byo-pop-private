// TypingTutorKeys.cpp: the finger and the key the tutor shows for the next character. The hands picture has nine fingers
// (0 the left little finger to 3 the left index finger, 4 the thumbs, 5 the right index finger to 8 the right little finger);
// the finger to use gets its darker picture and a ring with the key's name, a shifted character also lights the shift key of
// the other hand, and the key itself lights on the keyboard picture.
//
// Quirks of the original that are kept: the keys 8 and * light the picture of the 9 key at the place of the 8 key (the 8 key's
// picture is never used); the shift test does not list { } _ and |, which are typed with shift, so they light their key but
// not the shift key; the characters \ _ and | get no finger and no ring, while { and } get the finger of [ and ]; the tab and
// the other control characters light nothing. The tables are the original's own (two jump tables over the characters, the
// same for a letter in either case). The fixes mod's "tutorKeyboard" toggle mends them: the 8 key lights its own picture, the
// braces and the underscore light the shift key, and the underscore gets the right little finger and its ring (the lessons hold
// no bar or backslash).

#include "TypingTutor.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "Image.h"
#include "Res.h"
#include "Fixes.h"

using namespace Sexy;

// 0x45bf70: the nine fingers get their plain pictures (mImages 98 to 106) and their places on the hands picture; the rings
// around fingers go
void TypingTutor::ResetFingers()
{
	int aFingerIdx = 0;
	do
	{
		mFingers[aFingerIdx].mImage = mApp->mImages[IMAGE_FINGER0 + aFingerIdx];
		aFingerIdx++;
	} while (aFingerIdx < 9);

	mFingers[0].mX = 0x1a;
	mFingers[0].mY = 0x166;
	mFingers[1].mX = 0x35;
	mFingers[1].mY = 0x160;
	mFingers[2].mX = 0x58;
	mFingers[2].mY = 0x15d;
	mFingers[3].mX = 0x87;
	mFingers[3].mY = 0x161;
	mFingers[4].mX = 0xf8;
	mFingers[4].mY = 0x1ae;
	mFingers[5].mX = 0xff;
	mFingers[5].mY = 0x15f;
	mFingers[6].mX = 0x123;
	mFingers[6].mY = 0x15b;
	mFingers[7].mX = 0x14e;
	mFingers[7].mY = 0x15c;
	mFingers[8].mX = 0x176;
	mFingers[8].mY = 0x163;

	// 0x4433e0 for each: the labels go
	mLabels.clear();
}

// 0x45e810: what to show for the character that is next (mExpectedChar, taken from the task's current line). The fingers and the
// keyboard are reset, then: a character typed with the shift key lights the shift key of the other hand (the right shift key
// for the letters and symbols of the left hand, the left shift key for those of the right hand) and gives that hand's little
// finger its ring labelled with the shift key's name; the finger that types the character gets its darker picture and a ring
// labelled with the character (the thumb: the space bar); the key lights with the same label
void TypingTutor::SetupHighlight()
{
	std::string aLabel;				// the label of the sprites that are pushed: "Shift", then the character or "Space"
	int anImage;					// the picture of the sprite that is pushed, an image id
	int anX;
	int anY;

	// 0x4512a0: the old rings and keys go
	mLabels.clear();
	mGlows.clear();

	if (mLesson->mTasks.size() <= (unsigned int) mTask)
		return;

	TutorExercise* anExercise = mLesson->mTasks[mTask];
	mExpectedChar = anExercise->mTexts[anExercise->mCurText].c_str()[anExercise->mCurChar];
	ResetFingers();

	char aChar = mExpectedChar;
	bool aMend = FixEnabled("tutorKeyboard");
	// the fix: { } and _ are typed with shift too, so they take the right hand's branch of the switch below
	bool aShiftedSymbol = aMend && (aChar == '{' || aChar == '}' || aChar == '_');
	if (!aShiftedSymbol && (((((((aChar < 'A') || ('Z' < aChar)) && (aChar != '~')) && ((aChar != '!') && (aChar != '@'))) &&
		((aChar != '#') && ((aChar != '$') && (aChar != '%')))) && (aChar != '^')) &&
		(((((aChar != '&') && (aChar != '*')) && (aChar != '(')) &&
		(((aChar != ')') && (aChar != '+')) && ((aChar != '<') && ((aChar != '>') && (aChar != ':'))))) &&
		((aChar != '"') && (aChar != '?')))))
	{
		mShift = 0;
	}
	else
	{
		switch (aChar)
		{
		case '!': case '#': case '$': case '%': case '@':
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
		case 'Q': case 'R': case 'S': case 'T': case 'V': case 'W': case 'X': case 'Z':
		case '^': case '~':
			// the left hand's characters: the right shift key and the right little finger
			mShift = 1;
			mFingers[8].mImage = mApp->mImages[IMAGE_FINGER8X];
			aLabel = GT(0x00531778);		// "Shift"
			mLabels.push_back(TutorSprite(mApp->mImages[IMAGE_GLOWRING], aLabel, mFingers[8].mX, mFingers[8].mY));
			anImage = IMAGE_GLO_RSHIFT;
			anX = 0x174;
			break;
		default:
			// the right hand's: the left shift key and the left little finger
			mShift = -1;
			mFingers[0].mImage = mApp->mImages[IMAGE_FINGER0X];
			aLabel = GT(0x00531778);		// "Shift"
			mLabels.push_back(TutorSprite(mApp->mImages[IMAGE_GLOWRING], aLabel, mFingers[0].mX, mFingers[0].mY));
			anImage = IMAGE_GLO_LSHIFT;
			anX = 0x14;
		}
		anY = 0xfc;
		mGlows.push_back(TutorSprite(mApp->mImages[anImage], aLabel, anX, anY));
	}

	// 0x416490 / 0x416730: the character as a string, with the capital letters made small
	char aKey = aChar;
	if (('@' < aKey) && (aKey < '['))
		aKey = aKey + ' ';

	// the finger that types the key (the pictures of the darker fingers are IMAGE_FINGER0X to IMAGE_FINGER8X); the ring goes
	// round it with the key's name
	int aFinger = -1;
	switch (aKey)
	{
	case 10: case 13: case '"': case '\'': case ')': case '+': case '-': case '/': case '0': case ':': case ';':
	case '=': case '?': case '[': case ']': case 'p': case '{': case '}':
		aFinger = 8;
		aLabel = std::string(1, mExpectedChar);
		break;
	case ' ':
		aFinger = 4;
		aLabel = GT(0x00531770);			// "Space"
		break;
	case '!': case '1': case '`': case 'a': case 'q': case 'z': case '~':
		aFinger = 0;
		aLabel = std::string(1, mExpectedChar);
		break;
	case '#': case '3': case 'c': case 'd': case 'e':
		aFinger = 2;
		aLabel = std::string(1, mExpectedChar);
		break;
	case '$': case '%': case '4': case '5': case '^': case 'b': case 'f': case 'g': case 'r': case 't': case 'v':
		aFinger = 3;
		aLabel = std::string(1, mExpectedChar);
		break;
	case '&': case '6': case '7': case 'h': case 'j': case 'm': case 'n': case 'u': case 'y':
		aFinger = 5;
		aLabel = std::string(1, mExpectedChar);
		break;
	case '(': case '.': case '9': case '>': case 'l': case 'o':
		aFinger = 7;
		aLabel = std::string(1, mExpectedChar);
		break;
	case '*': case ',': case '8': case '<': case 'i': case 'k':
		aFinger = 6;
		aLabel = std::string(1, mExpectedChar);
		break;
	case '2': case '@': case 's': case 'w': case 'x':
		aFinger = 1;
		aLabel = std::string(1, mExpectedChar);
		break;
	default:
		// the control characters other than the new line, and the characters backslash, _ and |: no finger (the fix gives _ the right
		// little finger)
		if (aMend && aKey == '_')
		{
			aFinger = 8;
			aLabel = std::string(1, mExpectedChar);
		}
		break;
	}
	if (aFinger != -1)
	{
		mFingers[aFinger].mImage = mApp->mImages[IMAGE_FINGER0X + aFinger];
		mLabels.push_back(TutorSprite(mApp->mImages[IMAGE_GLOWRING], aLabel, mFingers[aFinger].mX, mFingers[aFinger].mY));
	}

	// the key that lights (the picture, its place on the keyboard picture); the capital letters and the characters outside
	// space to ~ light nothing of their own
	anImage = -1;
	anX = 0;
	anY = 0;
	switch (aKey)
	{
	case ' ':
		anImage = IMAGE_GLO_SPACE;
		anX = 119;
		anY = 278;
		break;
	case '!': case '1':
		anImage = IMAGE_GLO_1;
		anX = 64;
		anY = 171;
		break;
	case '"': case '\'':
		anImage = IMAGE_GLO_APOSTROPHE;
		anX = 350;
		anY = 224;
		break;
	case '#': case '3':
		anImage = IMAGE_GLO_3;
		anX = 117;
		anY = 171;
		break;
	case '$': case '4':
		anImage = IMAGE_GLO_4;
		anX = 141;
		anY = 171;
		break;
	case '%': case '5':
		anImage = IMAGE_GLO_5;
		anX = 166;
		anY = 171;
		break;
	case '&': case '7':
		anImage = IMAGE_GLO_7;
		anX = 215;
		anY = 171;
		break;
	case '(': case '9':
		anImage = IMAGE_GLO_9;
		anX = 264;
		anY = 171;
		break;
	case ')': case '0':
		anImage = IMAGE_GLO_0;
		anX = 289;
		anY = 171;
		break;
	case '*': case '8':
		anImage = aMend ? IMAGE_GLO_8 : IMAGE_GLO_9;
		anX = 237;
		anY = 171;
		break;
	case '+': case '=':
		anImage = IMAGE_GLO_PLUS;
		anX = 342;
		anY = 171;
		break;
	case ',': case '<':
		anImage = IMAGE_GLO_COMMA;
		anX = 283;
		anY = 252;
		break;
	case '-': case '_':
		anImage = IMAGE_GLO_HYPHEN;
		anX = 315;
		anY = 171;
		break;
	case '.': case '>':
		anImage = IMAGE_GLO_PERIOD;
		anX = 310;
		anY = 252;
		break;
	case '/': case '?':
		anImage = IMAGE_GLO_FORSLASH;
		anX = 341;
		anY = 252;
		break;
	case '2': case '@':
		anImage = IMAGE_GLO_2;
		anX = 91;
		anY = 171;
		break;
	case '6': case '^':
		anImage = IMAGE_GLO_6;
		anX = 190;
		anY = 171;
		break;
	case ':': case ';':
		anImage = IMAGE_GLO_COLON;
		anX = 321;
		anY = 224;
		break;
	case '[': case '{':
		anImage = IMAGE_GLO_LBRACE;
		anX = 328;
		anY = 197;
		break;
	case '\\': case '|':
		anImage = IMAGE_GLO_BACKSLASH;
		anX = 383;
		anY = 197;
		break;
	case ']': case '}':
		anImage = IMAGE_GLO_RBRACE;
		anX = 356;
		anY = 197;
		break;
	case '`': case '~':
		anImage = IMAGE_GLO_TILDE;
		anX = 36;
		anY = 171;
		break;
	case 'a':
		anImage = IMAGE_GLO_A;
		anX = 75;
		anY = 224;
		break;
	case 'b':
		anImage = IMAGE_GLO_B;
		anX = 201;
		anY = 252;
		break;
	case 'c':
		anImage = IMAGE_GLO_C;
		anX = 143;
		anY = 252;
		break;
	case 'd':
		anImage = IMAGE_GLO_D;
		anX = 133;
		anY = 224;
		break;
	case 'e':
		anImage = IMAGE_GLO_E;
		anX = 122;
		anY = 197;
		break;
	case 'f':
		anImage = IMAGE_GLO_F;
		anX = 160;
		anY = 224;
		break;
	case 'g':
		anImage = IMAGE_GLO_G;
		anX = 188;
		anY = 224;
		break;
	case 'h':
		anImage = IMAGE_GLO_H;
		anX = 213;
		anY = 224;
		break;
	case 'i':
		anImage = IMAGE_GLO_I;
		anX = 248;
		anY = 197;
		break;
	case 'j':
		anImage = IMAGE_GLO_J;
		anX = 238;
		anY = 224;
		break;
	case 'k':
		anImage = IMAGE_GLO_K;
		anX = 265;
		anY = 224;
		break;
	case 'l':
		anImage = IMAGE_GLO_L;
		anX = 292;
		anY = 224;
		break;
	case 'm':
		anImage = IMAGE_GLO_M;
		anX = 254;
		anY = 252;
		break;
	case 'n':
		anImage = IMAGE_GLO_N;
		anX = 227;
		anY = 252;
		break;
	case 'o':
		anImage = IMAGE_GLO_O;
		anX = 274;
		anY = 197;
		break;
	case 'p':
		anImage = IMAGE_GLO_P;
		anX = 300;
		anY = 197;
		break;
	case 'q':
		anImage = IMAGE_GLO_Q;
		anX = 67;
		anY = 197;
		break;
	case 'r':
		anImage = IMAGE_GLO_R;
		anX = 148;
		anY = 197;
		break;
	case 's':
		anImage = IMAGE_GLO_S;
		anX = 104;
		anY = 224;
		break;
	case 't':
		anImage = IMAGE_GLO_T;
		anX = 174;
		anY = 197;
		break;
	case 'u':
		anImage = IMAGE_GLO_U;
		anX = 223;
		anY = 197;
		break;
	case 'v':
		anImage = IMAGE_GLO_V;
		anX = 171;
		anY = 252;
		break;
	case 'w':
		anImage = IMAGE_GLO_W;
		anX = 94;
		anY = 197;
		break;
	case 'x':
		anImage = IMAGE_GLO_X;
		anX = 113;
		anY = 252;
		break;
	case 'y':
		anImage = IMAGE_GLO_Y;
		anX = 198;
		anY = 197;
		break;
	case 'z':
		anImage = IMAGE_GLO_Z;
		anX = 83;
		anY = 252;
		break;
	}
	if (anImage != -1)
		mGlows.push_back(TutorSprite(mApp->mImages[anImage], aLabel, anX, anY));

	mCleanChar = true;
}
