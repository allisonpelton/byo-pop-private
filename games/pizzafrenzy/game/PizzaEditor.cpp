#include "PizzaEditor.h"
#include "App.h"
#include "BonusGame.h"
#include "EditBox.h"
#include "GameText.h"
#include "Host.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"
#include "ToppingButton.h"

#include <cmath>

using namespace pizzafrenzy;

namespace
{
const char* const NODE_TOPPING_BUTTONS = "toppingButtons";
const char* const NODE_PIZZA_TOPPINGS = "targetToppings";
const char* const NODE_CAPTION = "caption";
const char* const NODE_NAME_ENTRY = "nameEntry";

// The actions of the two screens, and the ones the editor sends the application
const char* const ACTION_SNAP_TO_GRID = "editorSnapToGrid";
const char* const ACTION_EDIT_CAPTION = "editCaption";
const char* const ACTION_EXIT = "editorExit";
const char* const ACTION_PREVIOUS = "prevPizza";
const char* const ACTION_ADD = "addPizza";
const char* const ACTION_NEXT = "nextPizza";
const char* const ACTION_RESET = "resetPizza";
const char* const ACTION_DELETE = "deletePizza";
const char* const ACTION_NAME_DONE = "nameEntryDone";
const char* const ACTION_NAME_CANCEL = "nameEntryCancel";
const char* const ACTION_NAME_PIZZA = "namePizza";
const char* const ACTION_DIALOG_DONE = "done";

// The player's pizzas file
const char* const USER_PIZZAS_FILE = "userPizzas.xml";

// The buttons of the toppings stand in rows of this width, by this much, at this scale
const float BUTTON_SPACING = 60.0f;
const float BUTTON_ROW_WIDTH = 750.0f;
const float BUTTON_SCALE = 0.9f;

// A click puts a topping within this many pixels of the middle of the pizza; the cursor is faint beyond it; a click
// of the right button takes back the topping within the other range
const float PIZZA_RADIUS = 128.0f;
const float CURSOR_FAR_OPACITY = 0.5f;
const float TAKE_BACK_RANGE = 15.0f;

// The name entry cannot hold the file's quotation mark
const char QUOTE = '"';
const char REPLACEMENT_QUOTE = '\'';

double distanceOf(float theX, float theY)
{
	return std::sqrt((double)theX * theX + (double)theY * theY);
}
}

// 0x40f300
PizzaEditor::PizzaEditor(engine::GenericScreen* theScreen, engine::GenericScreen* theNameScreen,
                         std::vector<engine::Ref<Pizza>>* thePizzas, bool theDesigned) :
	mPizzas(thePizzas), mScreen(theScreen), mNameScreen(theNameScreen), mPizzaIndex(-1), mDesigned(theDesigned)
{
	mScreen->mActionSignal.connect(this, &PizzaEditor::action);
	mNameScreen->mActionSignal.connect(this, &PizzaEditor::action);
}

// 0x40f4b0, 0x40ef80
PizzaEditor::~PizzaEditor()
{
}

// 0x40e560
void PizzaEditor::setUp()
{
	mToppingButtons = static_cast<engine::Container*>(mScreen->getNode(NODE_TOPPING_BUTTONS));
	mPizzaToppingGroup = static_cast<engine::Container*>(mScreen->getNode(NODE_PIZZA_TOPPINGS));
	mCaption = static_cast<engine::TextItem*>(mScreen->getNode(NODE_CAPTION));
	mCursor = new engine::ImageNode();
	mScreen->addChild(mCursor);
	mPizzaIndex = (int)mPizzas->size() - 1;
}

// 0x40f9d0
void PizzaEditor::show()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mToppingButtons->removeAllChildren();
	UserGameData* aUser = anApp->mUserGameData;
	std::vector<engine::Ref<Topping>>& aToppings = anApp->mTileManifest->getToppingList();
	float aX = 0;
	float aY = 0;
	for (size_t i = 0; i < aToppings.size(); i++)
	{
		Topping* aTopping = aToppings[i];
		if (!mDesigned || aUser->getToppingLevel(aTopping) >= 0)
		{
			engine::Ref<ToppingButton> aButton = new ToppingButton(aTopping, -1, mScreen);
			aButton->setPosition(aX, aY);
			aButton->setScale(BUTTON_SCALE);
			mToppingButtons->addChild(aButton);
		}
		aX += BUTTON_SPACING;
		if (BUTTON_ROW_WIDTH <= aX)
		{
			aX = 0;
			aY += BUTTON_SPACING;
		}
	}
	if (mPizzaIndex >= 0)
		showPizza();
	else
		addPizza();
}

// 0x40fc20
void PizzaEditor::action(std::string theAction)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	if (theAction.compare(ACTION_SNAP_TO_GRID) == 0)
		return;
	if (theAction.compare(ACTION_EDIT_CAPTION) == 0)
	{
		static_cast<engine::EditBox*>(mNameScreen->getNode(NODE_NAME_ENTRY))->setText(mPizza->mName);
		anApp->screenAction(ACTION_NAME_PIZZA);
		selectTopping(nullptr);
		return;
	}
	if (theAction.compare(ACTION_EXIT) == 0)
	{
		savePizza();
		saveFile();
		anApp->screenAction(ACTION_EXIT);
		return;
	}
	if (theAction.compare(ACTION_PREVIOUS) == 0)
		showPreviousPizza();
	else if (theAction.compare(ACTION_ADD) == 0)
		addPizza();
	else if (theAction.compare(ACTION_NEXT) == 0)
		showNextPizza();
	else if (theAction.compare(ACTION_RESET) == 0)
		showPizza();
	else if (theAction.compare(ACTION_DELETE) == 0)
		deletePizza();
	else if (theAction.compare(ACTION_NAME_DONE) == 0)
	{
		std::string aName = static_cast<engine::EditBox*>(mNameScreen->getNode(NODE_NAME_ENTRY))->getText();
		if (!aName.empty())
		{
			for (size_t aQuote = aName.find(QUOTE); aQuote != std::string::npos; aQuote = aName.find(QUOTE, aQuote + 1))
				aName[aQuote] = REPLACEMENT_QUOTE;
			mPizza->mName = aName;
			showCaption();
			anApp->screenAction(ACTION_DIALOG_DONE);
		}
	}
	else if (theAction.compare(ACTION_NAME_CANCEL) == 0)
		anApp->screenAction(ACTION_DIALOG_DONE);
	else
	{
		// a topping's button sends the topping's name
		Topping* aTopping = anApp->mTileManifest->getTopping(theAction);
		if (aTopping != nullptr)
			selectTopping(aTopping);
	}
}

// 0x40f5b0
void PizzaEditor::showPizza()
{
	if (mPizzaIndex < (int)mPizzas->size() && mPizzaIndex >= 0)
	{
		mPizza = (*mPizzas)[mPizzaIndex];
		showCaption();
		mPlaced.clear();
		mPizzaToppingGroup->removeAllChildren();
		for (size_t i = 0; i < mPizza->mToppings.size(); i++)
			placeTopping(mPizza->mToppings[i].mPlace, mPizza->mToppings[i].mTopping);
	}
}

// 0x40e870
void PizzaEditor::showCaption()
{
	mCaption->setText(formatText(GT(0x4fa510), mPizzaIndex + 1, mPizza->mName));
}

// 0x40f4d0
void PizzaEditor::placeTopping(const engine::Point& thePlace, Topping* theTopping)
{
	engine::Ref<PizzaTopping> aPlaced = new PizzaTopping();
	aPlaced->setTopping(theTopping);
	mPlaced.push_back(aPlaced);
	mPizzaToppingGroup->addChild(aPlaced);
	aPlaced->setPosition(thePlace);
	aPlaced->setEnabled(false);
}

// 0x40e310: the places are whole pixels
void PizzaEditor::savePizza()
{
	if (mPizza == nullptr)
		return;
	mPizza->clearToppings();
	for (size_t i = 0; i < mPlaced.size(); i++)
	{
		engine::Point* aPosition = mPlaced[i]->getPosition();
		mPizza->addTopping(mPlaced[i]->getTopping(), engine::Point((float)(int)aPosition->x, (float)(int)aPosition->y));
	}
	PizzaFrenzy::instance()->addPizza(mPizza, mDesigned);
}

// 0x40f6b0
void PizzaEditor::addPizza()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	savePizza();
	engine::Ref<Pizza> aPizza = new Pizza();
	aPizza->mName = GT(0x4fa534);
	int anId = anApp->getPizzaMaxId(mDesigned);
	aPizza->mId = anId;
	anApp->setPizzaMaxId(anId + 1, mDesigned);
	anApp->addPizza(aPizza, mDesigned);
	mPizzaIndex = (int)mPizzas->size() - 1;
	mPizza = aPizza;
	showPizza();
}

// 0x40f670
void PizzaEditor::showNextPizza()
{
	savePizza();
	mPizzaIndex++;
	if (mPizzaIndex == (int)mPizzas->size())
		mPizzaIndex = 0;
	showPizza();
}

// 0x40f800
void PizzaEditor::showPreviousPizza()
{
	savePizza();
	mPizzaIndex--;
	if (mPizzaIndex < 0)
		mPizzaIndex = (int)mPizzas->size() - 1;
	showPizza();
}

// 0x40fb20
void PizzaEditor::deletePizza()
{
	if (!mPizzas->empty())
		mPizzas->erase(mPizzas->begin() + (mPizzaIndex > 0 ? mPizzaIndex : 0));
	if ((int)mPizzas->size() <= mPizzaIndex)
		mPizzaIndex = (int)mPizzas->size() - 1;
	mPizza = nullptr;
	if (!mPizzas->empty())
		showPizza();
	else
		addPizza();
}

// 0x40ebe0, 0x40e990: the file's element "pm" with the number the next pizza takes, and an element for each pizza
void PizzaEditor::saveFile()
{
	std::string aText = "<pm maxID=\"" + std::to_string(PizzaFrenzy::instance()->getPizzaMaxId(mDesigned)) + "\" >\n";
	for (size_t i = 0; i < mPizzas->size(); i++)
		aText += (*mPizzas)[i]->toXml();
	aText += "</pm>\n";
	engine::getHost()->writeUserFile(USER_PIZZAS_FILE, aText);
}

// 0x40f8b0
void PizzaEditor::selectTopping(Topping* theTopping)
{
	engine::App* anApp = engine::App::instance();
	if (mSelectedTopping != nullptr)
	{
		anApp->mMouseMoveSignal.disconnect(this);
		anApp->mMouseDownSignal.disconnect(this);
		anApp->mRightDownSignal.disconnect(this);
	}
	mSelectedTopping = theTopping;
	if (theTopping != nullptr)
	{
		mCursor->setImage(theTopping->mImage);
		anApp->mMouseMoveSignal.connect(this, &PizzaEditor::onMouseMove);
		anApp->mMouseDownSignal.connect(this, &PizzaEditor::onMouseDown);
		anApp->mRightDownSignal.connect(this, &PizzaEditor::onRightDown);
	}
	else
		mCursor->setImage(nullptr);
}

// 0x40e170
void PizzaEditor::onMouseMove(engine::Point& thePoint)
{
	engine::Point aGroup = BonusGame::getAbsPosition(mPizzaToppingGroup);
	float aDeltaX = (float)((int)thePoint.x - (int)aGroup.x);
	float aDeltaY = (float)((int)thePoint.y - (int)aGroup.y);
	mCursor->setOpacity(PIZZA_RADIUS <= distanceOf(aDeltaX, aDeltaY) ? CURSOR_FAR_OPACITY : 1.0f);
	mCursor->setPosition(thePoint);
}

// 0x40f840
void PizzaEditor::onMouseDown(engine::Point& thePoint)
{
	engine::Point aGroup = BonusGame::getAbsPosition(mPizzaToppingGroup);
	engine::Point aDelta((float)((int)thePoint.x - (int)aGroup.x), (float)((int)thePoint.y - (int)aGroup.y));
	if (distanceOf(aDelta.x, aDelta.y) < PIZZA_RADIUS && mSelectedTopping != nullptr)
		placeTopping(aDelta, mSelectedTopping);
}

// 0x40e940, 0x40e7b0
void PizzaEditor::onRightDown(engine::Point& thePoint)
{
	engine::Point aGroup = BonusGame::getAbsPosition(mPizzaToppingGroup);
	engine::Point aDelta((float)((int)thePoint.x - (int)aGroup.x), (float)((int)thePoint.y - (int)aGroup.y));
	for (size_t i = 0; i < mPlaced.size(); i++)
	{
		engine::Point* aPosition = mPlaced[i]->getPosition();
		if (distanceOf(aPosition->x - aDelta.x, aPosition->y - aDelta.y) < TAKE_BACK_RANGE)
		{
			mPizzaToppingGroup->removeChild(mPlaced[i]);
			mPlaced.erase(mPlaced.begin() + i);
			return;
		}
	}
}
