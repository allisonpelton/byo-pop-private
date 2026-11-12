#include "HelpWidget.h"
#include "BookWormApp.h"
#include "Res.h"
#include "Board.h"
#include "UserManager.h"
#include "Compat2004.h"
#include "GameText.h"
#include "ButtonWidget.h"
#include "Checkbox.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "Image.h"

using namespace Sexy;

// 0x52f128 the topic button labels
static const GameTextId gHelpTopicNames[HelpWidget::NUM_TOPICS] =
{
	GT_HELP_BUTTON_BONUS_WORDS,
	GT_HELP_BUTTON_REWARD_TILES,
	GT_HELP_BUTTON_SCORING,
	GT_HELP_BUTTON_ACTION_MODE,
	GT_HELP_BUTTON_HALL_OF_FAME
};

// 0x52f1d0 gHelpTopicText[5]: one paragraph per topic, built by the static initializer 0x4f7710 from the
// exe's strings at 0x4fd2f0 (bonus words), 0x4fd1d8 (reward tiles), 0x4fd118 (scoring), 0x4fd028 (action
// mode) and 0x4fcf68 (hall of fame); 284, 272, 184, 237 and 190 characters, no double spaces, newlines or
// colour codes.
static const GameTextId gHelpTopicTexts[HelpWidget::NUM_TOPICS] =
{
	GT_HELP_TOPIC_BONUS_WORDS,
	GT_HELP_TOPIC_REWARD_TILES,
	GT_HELP_TOPIC_SCORING,
	GT_HELP_TOPIC_ACTION_MODE,
	GT_HELP_TOPIC_HALL_OF_FAME
};

// 0x52f13c: topic label, label under the mouse
static int gHelpTopicColors[2][3] =
{
	{0, 0, 0},
	{0, 128, 0}
};

// 0x52f154: the label taking its turn in the attention cycle (brown), and under the mouse
static int gHelpTopicAttentionColors[2][3] =
{
	{126, 105, 78},
	{0, 128, 0}
};

// 0x429cd0
HelpWidget::HelpWidget(BookWormApp* theApp)
{
	mApp = theApp;
	mRefreshTopics = false;
	mUnused68 = 0;
	mUnused6C = 0;
	mTopicChosen = false;
	mShowHelpCheckbox = NULL;
	mContinueButton = NULL;
	mCheckedImage = NULL;
	mUncheckedImage = NULL;
	mFlippedArrow = NULL;
	mCurTopic = -1;
	for (int i = 0; i < NUM_TOPICS; i++)
		mTopicButtons[i] = NULL;
}

// 0x4297e0
HelpWidget::~HelpWidget()
{
	mTopicChosen = false;

	delete mShowHelpCheckbox;
	mShowHelpCheckbox = NULL;
	delete mContinueButton;
	mContinueButton = NULL;
	delete mCheckedImage;
	mCheckedImage = NULL;
	delete mUncheckedImage;
	mUncheckedImage = NULL;
	delete mFlippedArrow;
	mFlippedArrow = NULL;

	for (int i = 0; i < NUM_TOPICS; i++)
	{
		delete mTopicButtons[i];
		mTopicButtons[i] = NULL;
	}
}

// 0x429d90: pauses the board and makes the controls (top-level widgets above the help screen) on the
// first add
void HelpWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	if (mApp->mBoard != NULL)
		mApp->mBoard->Pause(true);

	Image* aCheckboxes = mApp->mImages[IMAGE_CHECKBOXES_HELP];
	int aHeight = aCheckboxes->GetHeight();
	int aWidth = aCheckboxes->GetWidth();
	if (mCheckedImage == NULL)
		mCheckedImage = mApp->CopyImage(aCheckboxes, Rect(aWidth / 2, 0, aWidth / 2, aHeight));
	if (mUncheckedImage == NULL)
		mUncheckedImage = mApp->CopyImage(aCheckboxes, Rect(0, 0, aWidth / 2, aHeight));
	if (mFlippedArrow == NULL)
	{
		// Made and freed, never drawn (original leftover)
		mFlippedArrow = mApp->CopyImage(mApp->mImages[IMAGE_BONUSARROW_RIGHT]);
		mApp->FlipImage(mFlippedArrow);
		mFlippedArrow->Palletize();
	}

	if ((mShowHelpCheckbox == NULL) && (mCheckedImage != NULL) && (mUncheckedImage != NULL))
	{
		mShowHelpCheckbox = new Checkbox(mUncheckedImage, mCheckedImage, SHOW_HELP_CHECKBOX_ID, this);
		mShowHelpCheckbox->Resize(0, 0, mUncheckedImage->GetWidth(), mUncheckedImage->GetHeight());
		mShowHelpCheckbox->mChecked = !mApp->mShowHelp;
		mShowHelpCheckbox->mHasAlpha = true;
	}

	if (mContinueButton == NULL)
	{
		ImageFont* aFont = mApp->mFonts[FONT_VERDANA12BOLD];
		Image* anImage = mApp->mImages[IMAGE_BUTTON_HELP_CONTINUE];
		mContinueButton = new ButtonWidget(CONTINUE_BUTTON_ID, this);
		mContinueButton->Resize(0, 0, anImage->GetWidth(), anImage->GetHeight());
		mContinueButton->mButtonImage = anImage;
		mContinueButton->SetFont(aFont);
		mContinueButton->mDoFinger = true;
	}

	if (mShowHelpCheckbox != NULL)
		theWidgetManager->AddWidget(mShowHelpCheckbox);
	if (mContinueButton != NULL)
		theWidgetManager->AddWidget(mContinueButton);

	for (int i = 0; i < NUM_TOPICS; i++)
	{
		if (mTopicButtons[i] == NULL)
		{
			ButtonWidget* aButton = new ButtonWidget(TOPIC_BUTTON_ID_FIRST + i, this);
			Image* anImage = mApp->mImages[IMAGE_BUTTON_HELP_TOPIC];
			aButton->mLabel = GameText(gHelpTopicNames[i]);
			aButton->SetFont(mApp->mFonts[FONT_MYRIADCONDENSEDWEB11]);
			aButton->Resize(74, anImage->GetHeight() * i + 284, anImage->GetWidth(), anImage->GetHeight());
			aButton->mDoFinger = true;
			aButton->mHasAlpha = true;
			// The chosen topic's button uses the wider hilite image (clipped to the button)
			aButton->mButtonImage = anImage;
			if (aButton->mId == mCurTopic)
				aButton->mButtonImage = mApp->mImages[IMAGE_BUTTON_HELP_TOPIC_HILITE];
			aButton->SetColors(gHelpTopicColors, 2);
			mTopicButtons[i] = aButton;
		}
		theWidgetManager->AddWidget(mTopicButtons[i]);
	}
}

// 0x4298d0
void HelpWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	if (mApp->mBoard != NULL)
		mApp->mBoard->Pause(false);
	mTopicChosen = false;

	Widget::RemovedFromManager(theWidgetManager);

	if (mShowHelpCheckbox != NULL)
		theWidgetManager->RemoveWidget(mShowHelpCheckbox);
	if (mContinueButton != NULL)
		theWidgetManager->RemoveWidget(mContinueButton);
	for (int i = 0; i < NUM_TOPICS; i++)
	{
		if (mTopicButtons[i] != NULL)
			theWidgetManager->RemoveWidget(mTopicButtons[i]);
	}
}

// 0x429940 OrderInManagerChanged (slot 3: puts the controls in front of the widget) is not ported: the help
// widget is added to its manager and removed, never moved in it.

// 0x429990: the controls follow the screen (BookWormApp::ShowHelp places it off screen first, then at 0,0)
void HelpWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	if (mShowHelpCheckbox != NULL)
		mShowHelpCheckbox->Move(mX + 215, mY + 395);

	if (mContinueButton != NULL)
		mContinueButton->Move((mWidth - mContinueButton->mWidth) / 2 + mX,
			(mHeight - mContinueButton->mHeight) - 24 + mY);

	for (int i = 0; i < NUM_TOPICS; i++)
	{
		ButtonWidget* aButton = mTopicButtons[i];
		if (aButton != NULL)
			aButton->Resize(mX + 74, aButton->mHeight * i + 284 + mY, aButton->mWidth, aButton->mHeight);
	}
}

// 0x429a50
void HelpWidget::Update()
{
	Widget::Update();

	if (mRefreshTopics && mTopicChosen)
	{
		// A topic was clicked: its button gets the hilite image, the others the normal one
		mRefreshTopics = false;
		for (int i = 0; i < NUM_TOPICS; i++)
		{
			ButtonWidget* aButton = mTopicButtons[i];
			if (aButton != NULL)
			{
				if (aButton->mId == mCurTopic)
					aButton->mButtonImage = mApp->mImages[IMAGE_BUTTON_HELP_TOPIC_HILITE];
				else
					aButton->mButtonImage = mApp->mImages[IMAGE_BUTTON_HELP_TOPIC];
			}
		}
		MarkDirty();
		return;
	}

	if (mTopicChosen)
		return;

	// Until a topic is clicked, every 32 updates: each label in turn (64 updates each) is drawn brown,
	// unless the mouse is over another topic button. The colours stay as they are once a topic is chosen.
	if (mUpdateCnt % 32 == 0)
	{
		int anAttentionId = (mUpdateCnt >> 6) % NUM_TOPICS + TOPIC_BUTTON_ID_FIRST;
		bool noOtherOver = true;
		ButtonWidget* anAttentionButton = NULL;
		for (int i = 0; i < NUM_TOPICS; i++)
		{
			ButtonWidget* aButton = mTopicButtons[i];
			if (aButton == NULL)
				continue;
			if (aButton->mId == anAttentionId)
			{
				anAttentionButton = aButton;
				continue;
			}
			aButton->SetColors(gHelpTopicColors, 2);
			if (aButton->mIsOver)
				noOtherOver = false;
		}
		if (noOtherOver && (anAttentionButton != NULL))
			anAttentionButton->SetColors(gHelpTopicAttentionColors, 2);
	}
}

// 0x4220c0 (shared with FameWidget)
void HelpWidget::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x429c70
void HelpWidget::ButtonDepress(int theId)
{
	if ((mContinueButton != NULL) && (theId == mContinueButton->mId))
	{
		SetVisible(false);
		SetDisabled(true);
		mApp->HideHelp(); // removes and safe-deletes this widget, shows and unpauses the board
	}

	if ((theId >= TOPIC_BUTTON_ID_FIRST) && (theId < TOPIC_BUTTON_ID_FIRST + NUM_TOPICS))
	{
		mCurTopic = theId;
		mTopicChosen = true;
		mRefreshTopics = true;
	}
}

// 0x42a740: the checkbox hides the screen at startup, the inverse of the option, saved for the player
void HelpWidget::CheckboxChecked(int theId, bool checked)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);

	if ((mShowHelpCheckbox != NULL) && (theId == SHOW_HELP_CHECKBOX_ID))
	{
		mApp->mShowHelp = !checked;
		SexyString aName = mApp->mUserManager->GetCurrentUserName();
		if (aName.length() != 0)
			mApp->mUserManager->WriteUserInt(aName, _S("ShowHelp"), mApp->mShowHelp ? 1 : 0);
	}
}

// 0x42a170
void HelpWidget::DrawTopicText(Graphics* g)
{
	Image* aBox = mApp->mImages[IMAGE_BACKDROP_HELP_BOX];

	if (!mTopicChosen || (mCurTopic < TOPIC_BUTTON_ID_FIRST) || (mCurTopic > TOPIC_BUTTON_ID_FIRST + NUM_TOPICS - 1))
	{
		// No topic yet: a prompt where the box would be, centred, without the box
		g->SetColor(Color(126, 105, 78));
		g->SetFont(mApp->mFonts[FONT_VERDANA10BOLD]);
		int aBoxHeight = aBox->GetHeight();
		int aYOffset = (int)(aBoxHeight * -0.4);
		Rect aRect(mX + 192, (mY - aYOffset) + 282, aBox->GetWidth() - 12, aBox->GetHeight() - 12);
		WriteWordWrapped2004(g, aRect, GameText(GT_HELP_CHOOSE_TOPIC), -1, 0);
		return;
	}

	int aTopic = mCurTopic - TOPIC_BUTTON_ID_FIRST;
	g->DrawImage(aBox, mX + 186, mY + 282);
	g->SetColor(Color(0, 0, 0));
	g->SetFont(mApp->mFonts[FONT_MYRIADCONDENSEDWEB11]);
	Rect aRect(mX + 192, mY + 288, aBox->GetWidth() - 12, aBox->GetHeight() - 12);

	SexyString aText = GameText(gHelpTopicTexts[aTopic]);
	// 0x42a2ea: for the Hall of Fame topic, a sentence pointing to the Hall of Fame's internet high-score link
	// is appended when IsInternetScoresEnabled(); not ported (internet scores, always off).

	WriteWordWrapped2004(g, aRect, aText, -1, -1);

	// The full hilite image under the chosen topic's button (the button draws its clipped copy and label on top)
	ButtonWidget* aButton = mTopicButtons[aTopic];
	if (aButton != NULL)
		g->DrawImage(mApp->mImages[IMAGE_BUTTON_HELP_TOPIC_HILITE], aButton->mX, aButton->mY);
}

// 0x42a420
void HelpWidget::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP_HELP], 0, 0);
	DrawTopicText(g);

	// Captions under the four pictures of the backdrop, centred
	g->SetColor(Color(0, 0, 0));
	g->SetFont(mApp->mFonts[FONT_MYRIADCONDENSEDWEB11]);
	WriteWordWrapped2004(g, Rect(mX + 72, mY + 218, 116, 48), GameText(GT_HELP_CAPTION_LINK), -1, 0);
	WriteWordWrapped2004(g, Rect(mX + 201, mY + 218, 114, 48), GameText(GT_HELP_CAPTION_SUBMIT), -1, 0);
	WriteWordWrapped2004(g, Rect(mX + 328, mY + 212, 118, 48), GameText(GT_HELP_CAPTION_BURNING), -1, 0);
	WriteWordWrapped2004(g, Rect(mX + 456, mY + 212, 116, 48), GameText(GT_HELP_CAPTION_SCRAMBLE), -1, 0);

	if (mShowHelpCheckbox != NULL)
	{
		g->SetColor(Color(0, 0, 0));
		g->DrawString(GameText(GT_HELP_DONT_SHOW), mShowHelpCheckbox->mWidth + 8 + mShowHelpCheckbox->mX,
			mShowHelpCheckbox->mHeight - 1 + mShowHelpCheckbox->mY);
	}
}
