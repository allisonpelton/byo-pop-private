#include "TitleScreen.h"

#include "CurtainTransition.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

TitleScreen::TitleScreen(GameApp* theApp)
{
	mFadeColor = Color::White;  // port: the fader (0x49e140) started from its own default
	mApp = theApp;
	mState = STATE_POPCAP_LOGO;
	mTimer = 0;
	mX = mY = mWidth = mHeight = 0;  // the original set them to -100000000; the app resizes it at once
	mHasTransparencies = true;
	mHasAlpha = true;
	mFirstWalk = true;
	mLex = nullptr;
	mLogoAlpha = 255;
	mPopCapLogoAlpha = 0;
	mFadeSpeed = mApp->GetInteger("PopCapLogoFadeSpeed", 2);
	// port: the partner logos (properties NumPartnerLogos, PartnerLogo<n> with an image path and its X and Y)
	// are not loaded: the game has none. The original also made a ButtonWidget (id 4) it never added.
	mLoaded = false;
	mEatWidth = mTileWidth = 0;
	mLexX = mLexY = mLexStartX = 0;
	mLogoY = 0;
}

TitleScreen::~TitleScreen()
{
	for (LoadingTile* aTile : mTiles) {
		delete aTile->mImage;
		delete aTile;
	}
	delete mLex;
}

bool TitleScreen::InitLoadingScreen()
{
	mLoaded = false;
	int aStartX = (mApp->mWidth - IMAGE_TS_TILE->mWidth * 7) / 2;
	int aTileWidth = IMAGE_TS_TILE->mWidth - 5;
	mEatWidth = aTileWidth * 7;
	mTileWidth = aTileWidth;
	// "LOADING", a letter a tile
	const SexyString* aLetters[7] = {&GT(0x6f7d28), &GT(0x6f821c), &GT(0x6f7d50), &GT(0x6f7d34), &GT(0x6f7d24),
	                                 &GT(0x6f7d40), &GT(0x6f8218)};
	static const int kY[7] = {475, 469, 479, 473, 469, 460, 468};
	for (int i = 0; i < 7; i++) {
		MemoryImage* anImage = new MemoryImage(mApp);
		anImage->Create(mTileWidth, IMAGE_TS_TILE->mHeight);
		Graphics g(anImage);
		g.SetColor(Color::Black);
		g.SetFont(FONT_TILE);
		const SexyString& aLetter = *aLetters[i];
		int aStringWidth = g.GetFont()->StringWidth(aLetter);
		int anAscent = g.GetFont()->GetAscent();
		g.DrawImage(IMAGE_TS_TILE, 0, 0);
		g.DrawString(aLetter, (anImage->mWidth - aStringWidth) / 2 + 3, (anImage->mHeight - anAscent) / 2 + 22);

		LoadingTile* aTile = new LoadingTile;
		aTile->mImage = anImage;
		aTile->mX = mTileWidth * i + aStartX;
		aTile->mY = kY[i];
		aTile->mDir = i % 2 != 0 ? -1 : 1;
		aTile->mOffset = aTile->mY - 470;
		mTiles.push_back(aTile);
	}

	mLex = new PopAnim(-1, this);
	if (!mLex->LoadAnimation("images/pam/lex.pam"))
		return false;
	mLex->PlayLabel("eat");
	mLexStartX = -120;
	mLexX = -120;
	mLexY = 220;
	mLogoY = -IMAGE_MM_LOGO->mHeight;
	return true;
}

void TitleScreen::Update()
{
	Widget::Update();
	MarkDirty();

	if (mState == STATE_LOADING) {
		mLex->UpdateF(1.0f);
		if (!mLoaded) {
			if (mUpdateCnt % 4 == 0) {
				for (LoadingTile* aTile : mTiles) {
					aTile->mOffset += aTile->mDir;
					aTile->mY += aTile->mDir;
					if (aTile->mOffset < -4 && aTile->mDir < 0) {
						aTile->mDir = -aTile->mDir;
						aTile->mY = aTile->mY - aTile->mOffset - 5;
					} else if (aTile->mOffset > 4 && aTile->mDir > 0) {
						aTile->mDir = -aTile->mDir;
						aTile->mY = aTile->mY - aTile->mOffset + 5;
					}
				}
			}
		}
		if (!mLoaded) {
			mLexX = (int)(mApp->GetLoadingThreadProgress() * mEatWidth + mLexStartX);
		} else {
			if (mLogoAlpha > 0) {
				mLogoAlpha -= 4;
				if (mLogoAlpha < 0)
					mLogoAlpha = 0;
			}
			mLexX += 3;
			if (mLexX >= 620)
				mApp->TitleScreenDone();
		}
		int aLogoRestY = (mHeight - IMAGE_MM_LOGO->mHeight) / 2 - 40;
		if (mLogoY < aLogoRestY) {
			mLogoY += 10;
			if (mLogoY > aLogoRestY)
				mLogoY = aLogoRestY;
		}
		return;
	}

	if (mState == STATE_POPCAP_LOGO) {
		if (mFadeSpeed > 0) {
			mPopCapLogoAlpha += mFadeSpeed;
			if (mPopCapLogoAlpha < 255)
				return;
			mPopCapLogoAlpha = 255;
			mFadeSpeed = -mFadeSpeed;
			mTimer = mApp->GetInteger("PopCapLogoTimer", 100);
			return;
		}
		if (mTimer > 0) {
			mTimer--;
			return;
		}
		mPopCapLogoAlpha += mFadeSpeed;
		if (mPopCapLogoAlpha > 0)
			return;
		mPopCapLogoAlpha = 0;
		// (with partner logos: STATE_PARTNER_LOGOS, fading their background in from the properties'
		// PartnerBGRed, PartnerBGGreen and PartnerBGBlue)
		mFadeColor = Color::White;
		mState = STATE_CURTAIN;
		StartCurtain();
	}
}

void TitleScreen::Draw(Graphics* g)
{
	if (mState == STATE_POPCAP_LOGO) {
		g->SetColor(Color::White);
		g->FillRect(0, 0, mWidth, mHeight);
		g->PushState();
		if (mPopCapLogoAlpha != 255) {
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, mPopCapLogoAlpha));
		}
		g->DrawImage(IMAGE_TS_POPCAP, (mWidth - IMAGE_TS_POPCAP->mWidth) / 2, (mHeight - IMAGE_TS_POPCAP->mHeight) / 2);
		g->PopState();
		return;
	}
	if (mState != STATE_LOADING) {
		g->SetColor(mFadeColor);
		g->FillRect(0, 0, mWidth, mHeight);
		return;
	}

	// For a first game the logo fades out as Lex walks off (the intro follows); else it stays for the main menu
	if (!mApp->HasPlayedGameBefore()) {
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mLogoAlpha));
	}
	g->DrawImage(IMAGE_MM_LOGO, (mWidth - IMAGE_MM_LOGO->mWidth) / 2, mLogoY);
	g->SetColorizeImages(false);
	// port: Volume 2's draw (0x48a940) keeps these colourize toggles but no longer draws Volume 1's "Deluxe" mark between them
	if (mLogoAlpha != 255) {
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mLogoAlpha));
		g->SetColorizeImages(false);
	}

	// What Lex has eaten of the tiles is not drawn
	int anEaten = (int)(mApp->GetLoadingThreadProgress() * mEatWidth);
	for (LoadingTile* aTile : mTiles) {
		if (anEaten < 1) {
			g->DrawImage(aTile->mImage, aTile->mX, aTile->mY);
			continue;
		}
		int anEatenHere = anEaten < mTileWidth ? anEaten : mTileWidth;
		anEaten -= anEatenHere;
		if (anEatenHere < mTileWidth)
			g->DrawImage(aTile->mImage, aTile->mX + anEatenHere, aTile->mY,
			             Rect(anEatenHere, 0, mTileWidth - anEatenHere, aTile->mImage->mHeight));
	}
	g->Translate(mLexX, mLexY);
	mLex->Draw(g);
	g->Translate(-mLexX, -mLexY);
}

// Lex chomps on while the game loads, then (after one more chomp) walks
void TitleScreen::PopAnimStopped(int theId)
{
	if (mApp->GetLoadingThreadProgress() < 1.0) {
		mLex->PlayLabel("eat");
		mApp->PlaySample(SOUND_CHOMP);
		return;
	}
	if (mFirstWalk) {
		mFirstWalk = false;
		mApp->PlaySample(SOUND_CHOMP);
	}
	mLex->PlayLabel("walk");
}

void TitleScreen::StartCurtain()
{
	mApp->mCurtainCallback = CurtainClosedCallback;
	mApp->mCurtainCallbackObject = this;
	mApp->mCurtain = mApp->GetCurtain();
	mApp->mCurtain->StartClosing();
	mApp->mTransitionState = GameApp::TRANSITION_TITLE_CURTAIN;
}

void TitleScreen::CurtainClosedCallback(void* theTitleScreen)
{
	((TitleScreen*)theTitleScreen)->CurtainClosed();
}

void TitleScreen::CurtainClosed()
{
	mState = STATE_LOADING;
	mApp->StartLoading();
	mApp->mWidgetManager->BringToFront(this);
}
