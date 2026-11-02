// Sexy::Tutorial (Tutorial.h).
#include "Tutorial.h"

#include "Board.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Piece.h"
#include "Profile.h"
#include "Res.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "WheelSpin.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

namespace {

const float kPi = 3.14159274f;     // 0x40490fdb
const float kTwoPi = 6.28318548f;  // 0x850b00

bool Is(const std::string& theWord, const char* theName)
{
	return sexystricmp(theWord.c_str(), theName) == 0;
}

// _wcsnicmp of the word's start against thePrefix
bool HasPrefix(const std::string& theWord, const char* thePrefix)
{
	size_t aLength = strlen(thePrefix);
	return theWord.size() >= aLength && sexystricmp(theWord.substr(0, aLength).c_str(), thePrefix) == 0;
}

// The script's word theIndex (the original's vector index would be out of range for a missing one)
const std::string& Word(const std::vector<std::string>& theWords, size_t theIndex)
{
	static const std::string kEmpty;
	return theIndex < theWords.size() ? theWords[theIndex] : kEmpty;
}

}  // namespace

Tutorial::Tutorial(Board* theBoard, Piece* thePiece, const char* theName, Buffer* theLines)
{
	mBoard = theBoard;
	mLines = theLines;
	theBoard->mUnknown2548 = 0xf;
	mState = 0;
	mUnknown210 = true;
	if (theName != nullptr)
		mGotoLabel = theName;
	mPointedPieceId = thePiece != nullptr ? thePiece->mId : -1;
}

Tutorial::~Tutorial()
{
	if (mDialog != nullptr)  // port: the original removes a null dialog without harm
		gApp->mWidgetManager->RemoveWidget(mDialog);
	delete mDialog;
	mDialog = nullptr;
	ClearDialog();
	delete mTitleImage;
	mTitleImage = nullptr;
	mTitleState = 0;
	RemoveAllWidgets(true, false);
}

void Tutorial::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mMouseVisible = true;
	CheckWait();
}

void Tutorial::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
}

void Tutorial::Update()
{
	TwistWidget::Update();
	if (std::find(mWidgets.begin(), mWidgets.end(), (Widget*)mFadingDialog) == mWidgets.end())
		mFadingDialog = nullptr;
	if (!mFinished)
		CheckWait();
	else if (mWidgets.empty())
		mClicked = true;
}

void Tutorial::Preload()
{
	mState = STATE_PRELOAD;
	do {
		if (!RunScript()) {
			mClicked = true;
			return;
		}
	} while (mState == STATE_PRELOAD);
}

void Tutorial::MoveDone()
{
	if (mState == STATE_MOVE)
		mMoveDone = true;
}

void Tutorial::PointAtNothing()
{
	mPointedPieceId = -1;
	mPointRow = -1;
	mPointCol = -1;
}

void Tutorial::ClearDialog()
{
	if (mDialog == nullptr)
		return;
	if (mAnim != nullptr) {
		mDialog->RemoveWidget(mAnim);
		gApp->SafeDeleteWidget(mAnim);
		mAnim = nullptr;
	}
	mPointRow = -1;
	mPointCol = -1;
	mFadingDialog = mDialog;
	mPointX = -10000;
	mPointY = -10000;
	mDialog->Close();
	mDialog = nullptr;
	mMouseVisible = false;
}

void Tutorial::PopAnimStopped(int)
{
	mAnim->PlayLabel(mAnimLabel);
}

void Tutorial::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId != 0x28)
		return;
	if (theButtonId == 1000) {
		mState = STATE_NONE;
	} else if (theButtonId == 1001) {
		// The replay button: the tip is not marked seen, and the move is played again
		mBoard->mApp->mProfile->SetFlag1E4(mBoard->mUnknown34E8, false);
		mMarkSeen = false;
		mState = STATE_NONE;
		mBoard->SaveGame(&mBoard->mUnknown23B4, false);
		mBoard->mUnknown3904 = false;
		// the move is rewound and made again (the exe takes the global board here, the same one)
		gBoard->StartReplay(gBoard->mSaveBuffer, gBoard->mUnknown23A4, gBoard->mUnknown23A8, true);
	}
}

bool Tutorial::ReadLine(std::string& theLine)
{
	if (mLines->AtEnd())
		return false;
	theLine = mLines->ReadString();
	mLineCount++;
	return true;
}

void Tutorial::CheckWait()
{
	Board* aBoard = mBoard;
	if (aBoard->IsBusy())
		mHint.clear();

	// The title: it fades in, holds, and shrinks into the corner (nothing ever makes the image)
	switch (mTitleState) {
	case 1: {
		if (1.0f <= mTitleAlpha) {
			mTitleHold = 1.0f;
			mTitleState = 2;
		} else {
			double aAlpha = (double)mTitleAlpha + (double)0.003f;
			mTitleAlpha = (float)(1.0 < aAlpha ? 1.0 : aAlpha);
		}
		break;
	}
	case 2:
		mTitleHold = (float)((double)mTitleHold - (double)0.01f);
		if (!(0.0f < mTitleHold))
			mTitleState = 3;
		break;
	case 3: {
		if (0.5f < mTitleScale) {
			double aScale = (double)mTitleScale - (double)0.005f;
			mTitleScale = (float)(aScale < 0.5 ? 0.5 : aScale);
		}
		double aMargin = (double)gApp->mScale * 15.0;
		float aDX = (float)(Ftol((double)mTitleImage->mWidth * (double)mTitleScale * 0.5 + aMargin) - (int)mTitleX);
		float aDY = (float)(Ftol((double)mTitleImage->mHeight * (double)mTitleScale * 0.5 + aMargin) - (int)mTitleY);
		float aDist = (float)std::sqrt(aDX * aDX + aDY * aDY);
		if (1.0f < aDist) {
			aDX /= aDist;
			mTitleX = (float)Ftol((double)aDX * 5.0 + (double)(int)mTitleX);
			aDY /= aDist;
			mTitleY = (float)Ftol((double)aDY * 5.0 + (double)(int)mTitleY);
		}
		if (aBoard->IsBusy()) {
			double aAlpha = (double)mTitleAlpha - (double)0.005f;
			mTitleAlpha = (float)(0.0 < aAlpha ? aAlpha : 0.0);
		}
		break;
	}
	default:
		break;
	}

	// The dialog glides to where the script put it, 15 pixels an update at most
	if (mDialog != nullptr) {
		if (mState != STATE_DIALOG && !mKeepDialog) {
			ClearDialog();
		} else if (0 <= mDialogX) {
			int aX = mDialog->mX;
			if (aX != mDialogX) {
				if (aX < mDialogX)
					aX = (mDialogX < aX + 15) ? mDialogX : aX + 15;
				else
					aX = (aX - 15 < mDialogX) ? mDialogX : aX - 15;
				mDialog->mX = aX;
			}
			int aY = mDialog->mY;
			if (aY != mDialogY) {
				if (aY < mDialogY)
					aY = (mDialogY < aY + 15) ? mDialogY : aY + 15;
				else
					aY = (aY - 15 < mDialogY) ? mDialogY : aY - 15;
				mDialog->mY = aY;
			}
		}
	}

	if (mState != STATE_NONE) {
		bool aDone = false;
		switch (mState) {
		case STATE_POINTS:
			aDone = (unsigned int)mStateArg <= (unsigned int)aBoard->mUnknown168;
			break;
		case STATE_MATCHES:
			if ((unsigned int)aBoard->mApp->mUnknownC74[41] < (unsigned int)mStateArg)
				return;
			mState = STATE_MATCHES_DONE;
			return;
		case STATE_COUNTERGEMS:
			aDone = (unsigned int)mStateArg <= (unsigned int)aBoard->mApp->mUnknownC74[71];
			break;
		case STATE_READY:
			aDone = !aBoard->IsBusy() && aBoard->mUnknown15C == 0 &&
				aBoard->mApp->mWidgetManager->mFocusWidget == aBoard && aBoard->mUnknown149C.empty() &&
				!aBoard->mUnknown21DA && !aBoard->mUnknown21D8 && aBoard->mUnknown25A4;
			break;
		case STATE_MOVE:
			if (!mMoveDone)
				return;
			aBoard->mUnknown2400 = -10000;
			aBoard->mUnknown2404 = -10000;
			mMoveDone = false;
			aDone = true;
			break;
		case STATE_DIALOG:
			aDone = mDialog == nullptr;
			break;
		case STATE_US:
			aDone = (unsigned int)mStateArg <= (unsigned int)aBoard->mApp->mUnknownC74[21];
			break;
		case STATE_LIGHTNINGGEMS:
			aDone = (unsigned int)mStateArg <= (unsigned int)aBoard->mApp->mUnknownC74[20];
			break;
		case STATE_MULTIPLIER:
			aDone = mStateArg <= aBoard->mUnknown21D0;
			break;
		case STATE_PRELOAD:
		case STATE_SHARDS:
			return;
		case STATE_LEVELUP:
			aDone = aBoard->IsBusy() ||
				(aBoard->mUnknown21A3 && aBoard->mUnknown25A4 && !aBoard->mUnknown21D8 && !aBoard->mUnknown21DA);
			break;
		case STATE_SECONDS:
			mTimer = (float)((double)mTimer + (double)0.01f);
			aDone = (double)mStateArg <= (double)mTimer * 100.0;
			break;
		case STATE_MATCHES_DONE:
			aDone = aBoard->mUnknown25A4;
			break;
		case STATE_TUBE:
			gApp->mYieldMainThread = true;
			mTimer = (float)((double)mTimer + (double)0.01f);
			if (!gApp->mTubeLoaded)
				return;
			gApp->mYieldMainThread = false;
			mUnknown210 = false;
			mBoard->ShowTube(true, false);
			aDone = true;
			break;
		case STATE_MENU:
			gApp->mYieldMainThread = true;
			mUnknown210 = true;
			mTimer = (float)((double)mTimer + (double)0.01f);
			if (!gApp->mLoadingThreadCompleted)
				return;
			gApp->mYieldMainThread = false;
			gApp->ShowMainMenu(false, false, 0.0f);
			return;
		default:
			return;
		}
		if (!aDone)
			return;
		mState = STATE_NONE;
		mHint.clear();
		mUnknown1C4 = 0;
		aBoard->EvaluateCursor(nullptr);
	}

	mArrowPhase = (float)((double)mArrowPhase + (double)0.04f);
	if (kTwoPi < mArrowPhase)
		mArrowPhase = mArrowPhase - kTwoPi;
	if (!RunScript())
		mFinished = true;
}

bool Tutorial::RunScript()
{
	Board* aBoard = mBoard;
	bool aGotLine = false;
	for (;;) {
		std::string aLine;
		if (!ReadLine(aLine))
			return aGotLine;
		aGotLine = true;
		// (the original prints the line with OutputDebugString)

		// The words of the line: spaces apart, "quotes" together, \n a new line and \\ a backslash
		std::vector<std::string> aWords;
		std::string aWord;
		bool aInWord = false;
		bool aInQuote = false;
		for (size_t i = 0; i < aLine.size(); i++) {
			char aChar = aLine[i];
			if (aChar == ' ') {
				if (!aInQuote) {
					if (aInWord) {
						aInWord = false;
						aInQuote = false;
						aWords.push_back(aWord);
						aWord.clear();
					}
				} else {
					aInWord = true;
					aWord += aChar;
				}
			} else if (aChar == '"') {
				if (aInQuote) {
					aInWord = false;
					aInQuote = false;
					aWords.push_back(aWord);
					aWord.clear();
				} else {
					aInQuote = true;
				}
			} else if (aChar == '\\') {
				size_t aNext = i + 1;
				char aNextChar = aNext < aLine.size() ? aLine[aNext] : '\0';
				if (aNextChar == 'n') {
					aWord += '\n';
					i = aNext;
				} else if (aNextChar == '\\') {
					aWord += '\\';
					i = aNext;
				}
			} else {
				aInWord = true;
				aWord += aChar;
			}
		}
		if (aInWord || aInQuote)
			aWords.push_back(aWord);

		if (aWords.empty())
			continue;
		if (1 < aWords[0].size() && aWords[0].compare(0, 2, "//") == 0)
			continue;
		if (aWords[0][0] == ':') {
			// A label: the lines before it are skipped until the one goto looks for
			if (Is(aWords[0].substr(1), mGotoLabel.c_str()))
				mGotoLabel.clear();
			continue;
		}
		if (!mGotoLabel.empty())
			continue;

		mUnknown210 = false;
		const std::string& aCommand = aWords[0];
		if (Is(aCommand, "end")) {
			return false;
		} else if (Is(aCommand, "mainmenu")) {
			gApp->mBoard->CloseTip();
			gApp->ShowMainMenu(false, false, 0.0f);
		} else if (Is(aCommand, "hint")) {
			if (1 < aWords.size())
				mHint = aWords[1];
			else
				mHint.clear();
		} else if (Is(aCommand, "textbox")) {
			if (aWords.size() == 1 || Is(aWords[1], "hide")) {
				ClearDialog();
				continue;
			}
			std::string aText = aWords[1];
			bool aNoButtons = false;
			bool aOkReplay = false;
			int aButtonMode = 1;
			for (size_t i = 2; i < aWords.size(); i++) {
				if (Is(aWords[i], "nobuttons")) {
					aNoButtons = true;
				} else if (Is(aWords[i], "okreplay")) {
					aOkReplay = true;
					aButtonMode = 3;
				}
			}
			bool aCreated = false;
			if (mDialog == nullptr) {
				mDialog = new TwistDialog(IMAGE_INGAMEUI_TOOLTIPDIALOG, IMAGE_INGAMEUI_TOOLTIPBUTTON, 0x28, false,
					_S(""), aText, _S(""), 0);
				mDialog->SetLinesFont(FONT_TUTORIAL);
				mDialog->mMouseDownOn = false;
				mDialog->mTwistListener = this;
				mDialog->mPriority = 100;
				gApp->mWidgetManager->AddWidget(mDialog);
				aCreated = true;
			} else {
				mDialog->mDialogLines = aText;
			}
			mDialog->PopUp();
			mDialog->DeleteButtons();
			if (!aNoButtons)
				mDialog->CreateButtons(aButtonMode);
			if (aOkReplay) {
				mDialog->mYesButton->mLabel = gApp->GetString(GTS(0x7f1f54), GT(0x8144e0));
				mDialog->mNoButton->mLabel = gApp->GetString(GTS(0x7f1f48), GT(0x7f1f30));
				mDialog->LayoutButtons();
			}
			mMouseVisible = !aNoButtons;
			mUnknown210 = !aNoButtons;
			int aWidth = gGemWidth * 7;
			mDialog->mButtonBottom = Ftol((double)gApp->mScale * 48.0);
			int aInset = Ftol((double)gApp->mScale * 40.0);
			mDialog->mContentInsets.mLeft = aInset;
			mDialog->mContentInsets.mTop = aInset;
			mDialog->mContentInsets.mRight = aInset;
			mDialog->mContentInsets.mBottom = aInset;
			int aHeight = mDialog->GetPreferredHeight(aWidth);
			int aHalfWidth = aWidth / 2;
			int aX = mWidth / 2 - aHalfWidth;
			int aY = Ftol((double)gGemHeight * 0.35);
			bool aGraduate = false;
			for (size_t i = 2; i < aWords.size(); i++) {
				const std::string& aOption = aWords[i];
				if (Is(aOption, "vcenter")) {
					aY = Ftol((double)mHeight * 0.4 - (double)(aHeight / 2));
				} else if (Is(aOption, "left")) {
					aX = Ftol((double)mWidth * (double)0.05f);
				} else if (Is(aOption, "right")) {
					aX = Ftol((double)mWidth * (double)0.95f - (double)aWidth);
				} else if (Is(aOption, "bottom")) {
					aY = Ftol((double)mHeight * (double)0.95f - (double)aHeight);
				} else if (HasPrefix(aOption, "space:")) {
					int aSpace = atoi(aOption.substr(6).c_str());
					mDialog->mContentInsets.mTop += Ftol((double)gApp->mScale * (double)aSpace);
					aHeight = mDialog->GetPreferredHeight(aWidth);
				} else if (Is(aOption, "graduate")) {
					aGraduate = true;
				} else if (HasPrefix(aOption, "anim:")) {
					// anim:file,label,x,y,width,height: a PopAnim in the dialog (x 0 centres it)
					if (mAnim != nullptr) {
						mAnim->mParent->RemoveWidget(mAnim);
						gApp->SafeDeleteWidget(mAnim);
						mAnim = nullptr;
					}
					mAnim = new PopAnim(0, this);
					mAnim->mImageFolders.clear();
					mAnim->mImageFolders.push_back(StrFormat(GTC(0x7d9bac), gApp->mArtRes));
					mAnim->mImageScale = gApp->mScale;
					mAnim->mDrawScale = gApp->mScale;
					int aAnimX = 0, aAnimY = 0, aAnimW = 0, aAnimH = 0;
					int aToken = 0;
					size_t aPos = 5;
					if (5 < aOption.size()) {
						do {
							size_t aComma = aOption.find(',', aPos);
							size_t aEnd = aComma == std::string::npos ? aOption.size() : aComma;
							std::string aPart = aOption.substr(aPos, aEnd - aPos);
							if (aToken == 0) {
								mAnim->LoadAnimation(aPart);
							} else if (aToken == 1) {
								mAnimLabel = aPart;
							} else if (aToken == 2) {
								aAnimX = Ftol((double)gApp->mScale * (double)atoi(aPart.c_str()));
							} else if (aToken == 3) {
								aAnimY = Ftol((double)gApp->mScale * (double)atoi(aPart.c_str()));
							} else if (aToken == 4) {
								aAnimW = Ftol((double)gApp->mScale * (double)atoi(aPart.c_str()));
							} else if (aToken == 5) {
								aAnimH = Ftol((double)gApp->mScale * (double)atoi(aPart.c_str()));
							}
							aToken++;
							aPos = aEnd + 1;
						} while (aPos < aOption.size());
					}
					if (aAnimX == 0)
						aAnimX = (aWidth - aAnimW) / 2;
					mDialog->AddWidget(mAnim);
					mAnim->PlayLabel(mAnimLabel);
					mAnim->Resize(Rect(aAnimX, aAnimY, aAnimW, aAnimH));
				} else if (isdigit((unsigned char)aOption[0])) {
					int aRow = 0, aCol = 0;
					sscanf(aOption.c_str(), "%d,%d", &aRow, &aCol);
					aRow--;
					aCol--;
					aX = Ftol((double)gGemWidth * 0.25 + (double)aBoard->GetColX(aCol));
					aY = Ftol((double)aBoard->GetRowY(aRow) - (double)gGemHeight * 0.25);
				}
			}
			// port: the original ends every textbox with the board's centre, whatever the options set above: x is
			// the centre less half the width, y is 0 (400 for graduate) in screen units
			aBoard->GetCenter(&aX, &aY);
			aY = Ftol((double)gApp->mScale * (aGraduate ? 400.0 : 0.0));
			aX -= aHalfWidth;
			Piece* aPiece = aBoard->GetPieceById(mPointedPieceId);
			if (aPiece != nullptr) {
				double aNewY;
				if ((float)(aBoard->mHeight / 2) <= aPiece->mY) {
					double aAbove = ((double)aPiece->mY - (double)gGemHeight * 0.5) - (double)aHeight;
					aNewY = 0.0 < aAbove ? aAbove : 0.0;
				} else {
					double aBelow = (double)gGemHeight * 1.5 + (double)aPiece->mY;
					double aLimit = (double)(mHeight - aHeight) - (double)(float)((double)gApp->mScale * 10.0);
					aNewY = aBelow < aLimit ? aBelow : aLimit;
				}
				aY = Ftol(aNewY);
			}
			mDialogX = aX;
			mDialogY = aY;
			if (!aCreated)
				mDialog->Resize(mDialog->mX, mDialog->mY, aWidth, aHeight);
			else
				mDialog->Resize(aX, aY, aWidth, aHeight);
			if (aGraduate) {
				gApp->PlaySample(SOUND_TUTORIALCOMPLETE);
				for (int i = 250; i != 0; i--) {
					Effect* aEffect = aBoard->mFXManager->AllocEffect(0x28);
					float anAngle = RandSigned() * kPi;
					float aBase = (float)((double)gApp->mScale * 5.0);
					float aRange = (float)((double)gApp->mScale * 12.0);
					float aSpeed = (float)((double)RandFloat() * (double)aRange + (double)aBase);
					aEffect->mDAlpha = -0.001f;
					aEffect->mDScale = -0.0075f;
					aEffect->mX = (float)(mDialog->mWidth / 2 + mDialog->mX);
					aEffect->mY = (float)Ftol((double)gApp->mScale * 500.0);
					aEffect->mDX = (float)((double)cosf(anAngle) * (double)aSpeed);
					aEffect->mScale = 1.5f;
					aEffect->mDY = (float)((double)sinf(anAngle) * (double)aSpeed * (double)0.65f);
					aEffect->mDXScalar = 0.98f;
					aEffect->mDYScalar = 0.98f;
					aEffect->mIsAdditive = true;
					aEffect->mAngle = RandSigned() * kPi;
					aEffect->mColor = Color::White;
					aBoard->mFXManager->AddEffect(aEffect);
				}
			}
			mKeepDialog = aNoButtons;
			if (!aNoButtons)
				mState = STATE_DIALOG;
		} else if (Is(aCommand, "tube")) {
			if (Is(Word(aWords, 1), "hide")) {
				if (aWords.size() < 3) {
					aBoard->ShowTube(false, false);
				} else if (Is(aWords[2], "rank")) {
					aBoard->mGoodTube->SetShown(false);
				} else if (Is(aWords[2], "chains")) {
					aBoard->mGoodTube->SetUnknown14D(false);
				}
			} else if (Is(Word(aWords, 1), "show")) {
				mTimer = 0;
				mState = STATE_TUBE;
				mUnknown210 = true;
				if (aWords.size() < 3) {
					for (int aRow = 0; aRow < aBoard->mRows; aRow++) {
						for (int aCol = 0; aCol < aBoard->mCols; aCol++) {
							Piece* aPiece = aBoard->GetPieceAt(aRow, aCol);
							if (aPiece != nullptr) {
								aPiece->mX = (float)aBoard->GetColX(aCol);
								aPiece->mY = (float)aBoard->GetRowY(aRow);
							}
						}
					}
				} else if (Is(aWords[2], "rank")) {
					aBoard->mGoodTube->SetShown(true);
				} else if (Is(aWords[2], "chains")) {
					aBoard->mGoodTube->SetUnknown14D(true);
				}
			}
		} else if (Is(aCommand, "loadboard")) {
			aBoard->ClearBoard();
			aBoard->FillInBlanks(false);
			if (mState == STATE_PRELOAD)
				mState = STATE_READY;
		} else if (Is(aCommand, "row")) {
			// row <gems>: the next gem of each column, a hex digit each (c for a flame gem 9, x for a star gem 0xb in
			// front of the digit)
			const std::string& aRowText = Word(aWords, 1);
			if (!aRowText.empty()) {
				int aGem = 0;
				int aColumn = 0;  // (the original counts 0x10 a column, the size of a std::vector there)
				for (size_t i = 0; i < aRowText.size(); i++) {
					unsigned int aChar = (unsigned char)aRowText[i];
					if (!isalpha((int)aChar)) {
						int aValue = (int)((unsigned int)aGem << 4 | (aChar - '0'));
						aBoard->mUnknown2408[aColumn].push_back(aValue);
						aGem = 0;
						aColumn++;
					} else {
						int aLower = tolower((int)aChar);
						if (aLower == 'c')
							aGem |= 9;
						else if (aLower == 'x')
							aGem |= 0xb;
					}
				}
			}
		} else if (Is(aCommand, "matchdelay")) {
			if (aWords.size() < 2) {
				aBoard->mUnknown23EC = 0;
			} else {
				sscanf(aWords[1].c_str(), "%f", &aBoard->mUnknown23EC);
			}
		} else if (Is(aCommand, "forcemove")) {
			if (aWords.size() < 2) {
				aBoard->mUnknown2404 = -10000;
				aBoard->mUnknown2400 = -10000;
			} else {
				int aRow = 0, aCol = 0;
				sscanf(aWords[1].c_str(), "%d,%d", &aRow, &aCol);
				aRow--;
				aCol--;
				if (aBoard->mUnknown23EC != 0.0f) {
					// Find the matches the move would make, flag their gems, and put the board back
					Buffer aSaved;
					aBoard->SaveGame(&aSaved, false);
					aBoard->StartTwist(true, aBoard->GetColX(aCol), aBoard->GetRowY(aRow));
					aBoard->FindMatches(1);
					for (int aTestRow = 0; aTestRow < aBoard->mRows; aTestRow++) {
						for (int aTestCol = 0; aTestCol < aBoard->mCols; aTestCol++) {
							Piece* aPiece = aBoard->GetPieceAt(aTestRow, aTestCol);
							if (aPiece != nullptr && aPiece->mUnknownEC)
								aPiece->mUnknown14 = 1.0f;
						}
					}
					aBoard->ClearSparkStarts();
					aBoard->LoadGame(aSaved);
				}
				aBoard->mUnknown2400 = aCol;
				aBoard->mUnknown2404 = aRow;
				mState = STATE_MOVE;
				mMoveDone = false;
				// 0x75fc8a
				if (gApp->mBoard == nullptr || !gApp->mBoard->mUnknown37C4)
					std::memset(gApp->mUnknownC74, 0, sizeof(gApp->mUnknownC74));
				aBoard->EvaluateCursor(nullptr);
			}
		} else if (Is(aCommand, "swapspeed")) {
			if (1 < aWords.size())
				sscanf(aWords[1].c_str(), "%f", &aBoard->mUnknown145C);
		} else if (Is(aCommand, "waitfor")) {
			if (aWords.size() < 2)
				continue;
			if (Is(aWords[1], "ready")) {
				mState = STATE_READY;
			} else if (Is(aWords[1], "levelup")) {
				mState = STATE_LEVELUP;
				mStateArg = aBoard->mUnknown158 + 1;
			} else if (Is(aWords[1], "preload")) {
				mState = STATE_PRELOAD;
			} else if (Is(aWords[1], "tube")) {
				mState = STATE_TUBE;
				mTimer = 0;
			} else if (2 < aWords.size()) {
				const std::string& aWhat = aWords[2];
				if (Is(aWhat, "points")) {
					mState = STATE_POINTS;
				} else if (Is(aWhat, "matches")) {
					mState = STATE_MATCHES;
					mStateArg = atoi(aWords[1].c_str()) + aBoard->mApp->mUnknownC74[41];
					return true;
				} else if (Is(aWhat, "countergems")) {
					mState = STATE_COUNTERGEMS;
				} else if (Is(aWhat, "us")) {
					mState = STATE_US;
				} else if (Is(aWhat, "lightninggems")) {
					mState = STATE_LIGHTNINGGEMS;
				} else if (Is(aWhat, "multiplier")) {
					mState = STATE_MULTIPLIER;
				} else if (Is(aWhat, "shards")) {
					mState = STATE_SHARDS;
				} else if (Is(aWhat, "seconds")) {
					mState = STATE_SECONDS;
					mStateArg = Ftol(atof(aWords[1].c_str()) * 100.0);
					mTimer = 0;
					mUnknown210 = true;
					return true;
				} else {
					return true;
				}
				mStateArg = atoi(aWords[1].c_str());
			}
			return true;
		} else if (Is(aCommand, "levelup")) {
			if (1 < aWords.size())
				aBoard->mUnknown158 = atoi(aWords[1].c_str());
			// 0x6a63a1(level + 1): +0x200 = +0x204, level = (level + 1) - 1 and +0x21a3 set
			aBoard->mUnknown200 = aBoard->mUnknown204;
			aBoard->mUnknown21A3 = true;
			mState = STATE_PRELOAD;
		} else if (Is(aCommand, "rankup")) {
			mState = STATE_READY;
		} else if (Is(aCommand, "pointat")) {
			if (aWords.size() < 2)
				continue;
			const std::string& aWhat = aWords[1];
			GoodTube* aTube = aBoard->mGoodTube;
			double aY = 0.0;
			bool aPoint = true;
			if (Is(aWhat, "tube")) {
				mPointX = Ftol((double)aTube->mWidth * (double)0.83f + (double)aTube->mX);
				aY = (double)aTube->mHeight * (double)0.605f;
			} else if (Is(aWhat, "points")) {
				mPointX = Ftol((double)aTube->mWidth * (double)0.85f + (double)aTube->mX);
				aY = (double)aTube->mHeight * 0.25;
			} else if (Is(aWhat, "chains")) {
				mPointX = Ftol((double)aTube->mWidth * (double)0.85f + (double)aTube->mX);
				aY = (double)aTube->mHeight * (double)0.15f;
			} else if (Is(aWhat, "rank")) {
				mPointX = Ftol((double)aTube->mX + (double)aTube->mWidth * (double)0.85f);
				aY = (double)0.85f * (double)aTube->mHeight;
			} else if (Is(aWhat, "shards")) {
				mPointX = Ftol((double)aTube->mWidth * 1.25 + (double)aTube->mX);
				aY = (double)aTube->mHeight;
			} else {
				aPoint = false;
				if (isdigit((unsigned char)aWhat[0])) {
					int aRow = 0, aCol = 0;
					sscanf(aWhat.c_str(), "%d,%d", &aRow, &aCol);
					mPointCol = aCol - 1;
					mPointRow = aRow - 1;
				}
			}
			if (aPoint) {
				mPointY = Ftol(aY + (double)aTube->mY);
				mPointAngle = kPi;
			}
		} else if (Is(aCommand, "tubegoal")) {
			aBoard->mUnknown204 = atoi(Word(aWords, 1).c_str());
		} else if (Is(aCommand, "countergem")) {
			aBoard->mUnknown2548 = atoi(Word(aWords, 1).c_str());
		} else if (Is(aCommand, "unlock")) {
			if (aWords.size() < 2)
				continue;
			Profile* aProfile = aBoard->mApp->mProfile;
			const std::string& aWhat = aWords[1];
			if (Is(aWhat, "flamegem"))
				aProfile->SetFlag1E0(4, true);
			else if (Is(aWhat, "lightninggem"))
				aProfile->SetFlag1E0(3, true);
			else if (Is(aWhat, "icegem"))
				aProfile->SetFlag1E0(6, true);
			else if (Is(aWhat, "angelgem"))
				aProfile->SetFlag1E0(7, true);
			else if (Is(aWhat, "classic"))
				aProfile->UnlockMode(1);
			else if (Is(aWhat, "zen"))
				aProfile->UnlockMode(4);
			else if (Is(aWhat, "puzzle"))
				aProfile->UnlockMode(5);
			aProfile->Save();
		} else if (Is(aCommand, "awardfruit")) {
			int aA = -1, aB = -1;
			if (1 < aWords.size())
				sscanf(aWords[1].c_str(), "%d,%d", &aA, &aB);
			aBoard->AwardFruit(aA - 1, aB - 1);
		} else if (Is(aCommand, "points")) {
			aBoard->SetScore(atoi(Word(aWords, 1).c_str()), false);
			aBoard->mUnknown14F8 = aBoard->mUnknown168;
			aBoard->mUnknown200 = aBoard->mUnknown14F8;
			if (aBoard->mUnknown168 == 0)
				aBoard->SetLevelSteps(0);
		} else if (Is(aCommand, "level")) {
			aBoard->mUnknown158 = atoi(Word(aWords, 1).c_str());
		} else if (Is(aCommand, "title")) {
			// (the original also copies the word without its backslashes into a string it drops)
			delete mTitleImage;
			mTitleImage = nullptr;
		} else if (Is(aCommand, "goto")) {
			if (mGotoLabel.empty() && 1 < aWords.size()) {
				mGotoLabel = aWords[1];
				mLines->SeekFront();
				mState = STATE_NONE;
			}
		} else if (Is(aCommand, "moves")) {
			if (Is(Word(aWords, 1), "allowed"))
				mMovesAllowed = true;
			else if (Is(Word(aWords, 1), "disallowed"))
				mMovesAllowed = false;
		}
		if (mState != STATE_NONE)
			return true;
	}
}

void Tutorial::Draw(Graphics* g)
{
	if (mClicked)
		return;
	if (std::find(mWidgets.begin(), mWidgets.end(), (Widget*)mFadingDialog) == mWidgets.end() && mFinished)
		return;
	mWidgetManager->FlushDeferredOverlayWidgets(100);

	if (!mHint.empty()) {
		g->SetFont(FONT_HUGE);
		int aCenterX, aCenterY;
		mBoard->GetCenter(&aCenterX, &aCenterY);
		int aX = aCenterX - g->StringWidth(mHint) / 2;
		Font* aFont = g->GetFont();
		int aHeight = aFont->GetHeight();
		int aAscent = aFont->GetAscent();
		int aY = Ftol((double)(aHeight - aAscent) * 1.25);
		g->SetColor(Color::Black);
		g->DrawString(mHint, aX + 4, aY + 4);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(mHint, aX, aY);
	}

	// The arrow and the hole fade with the dialog
	g->SetColorizeImages(true);
	int anAlpha = 255;
	if (mDialog != nullptr) {
		anAlpha = Ftol((double)mDialog->mPopScale * 255.0);
		if (255 < anAlpha)
			anAlpha = 255;
	} else if (mFadingDialog != nullptr) {
		anAlpha = Ftol((double)mFadingDialog->mPopScale * 512.0);
		if (255 < anAlpha)
			anAlpha = 255;
		else if (!(0 < anAlpha))
			anAlpha = 0;
	}
	g->SetColor(Color(255, 255, 255, anAlpha));

	float aPhase = (float)((double)(mUpdateCnt % 100) / 100.0);
	aPhase = aPhase * kTwoPi;
	float aSin = sinf(aPhase);
	float aCos = cosf(mPointAngle);
	int aArrowX = Ftol((double)aCos * (double)aSin * 20.0 + (double)mPointX);
	float aSinAngle = sinf(mPointAngle);
	int aArrowY = Ftol((double)aSinAngle * (double)aSin * 20.0 + (double)mPointY);
	float aArrowScale = gApp->Is3DAccelerated() ? 0.85f : 1.0f;
	DrawImageCentered(g, IMAGE_TUTORIAL_ARROW, (float)aArrowX, (float)aArrowY, (double)mPointAngle, aArrowScale,
		aArrowScale);

	if (-1 < mPointRow || -1 < mPointedPieceId) {
		float aAlphaF = (float)((double)g->mColor.mAlpha / 255.0);
		Piece* aPiece = -1 < mPointedPieceId ? mBoard->GetPieceById(mPointedPieceId) : nullptr;
		float aDark = (float)((double)aAlphaF * (double)0.78f);
		int aHoleX, aHoleY;
		if (aPiece != nullptr) {
			aHoleX = Ftol((double)(float)(gGemWidth / 2) + (double)aPiece->mX);
			aHoleY = Ftol((double)(float)(gGemHeight / 2) + (double)aPiece->mY);
			if (aPiece->IsFlagSet(10))
				aDark = (float)((double)aAlphaF * (double)0.85f);
		} else {
			aHoleX = mBoard->GetColX(mPointCol) + gGemWidth / 2;
			aHoleY = mBoard->GetRowY(mPointRow) + gGemHeight / 2;
		}
		if (mBoard->mUnknown267C == nullptr) {
			float aSize = (float)(3.0 - ((double)aAlphaF - 1.0) * 2.0);
			WheelSpin::DrawHole(g, aHoleX, aHoleY, aDark, aSize);
		}
	}

	if (mTitleImage != nullptr && mTitleState != 0) {
		// port: 0x751111 draws the title's image, stretched, in black and then white; nothing ever makes the image
		NotPorted("Tutorial::Draw: the title (0x751111)");
	}
	DeferOverlay(1000);
}

void Tutorial::DrawOverlay(Graphics* g)
{
	if (!((mState == STATE_TUBE || mState == STATE_MENU) && 0.02f < mTimer))
		return;
	int aWidth = Ftol((double)gApp->mWidth * (double)0.2f);
	int aHeight = Ftol((double)0.2f * (double)gApp->mHeight);
	SexyString aText = gApp->GetString(GTS(0x7f1c0c), GT(0x7f1bf4));
	g->SetColor(Color(0, 0, 0, 192));
	g->FillRect(gApp->mWidth / 2 - aWidth / 2, gApp->mHeight / 2 - aHeight / 2, aWidth, aHeight);
	g->SetColor(Color::White);
	g->SetFont(FONT_TUTORIAL);
	g->DrawString(aText, gApp->mWidth / 2 - g->StringWidth(aText) / 2, gApp->mHeight / 2);
}
