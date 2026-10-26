#include "XLayout.h"

#include <SDL.h>

#include <cstdlib>
#include <set>

#include "Common.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "ResourceManager.h"
#include "SexyAppBase.h"
#include "XMLParser.h"

using namespace Sexy;

namespace {

// 0x56cda0: a key or value in capitals
std::string Upper(const std::string& theString)
{
	return StringToUpper(theString);
}

// the Control tag's flags: "true" in any case
bool IsTrue(const std::string& theValue)
{
	return Upper(theValue) == "TRUE";
}

Image* FindImage(const std::string& theName)
{
	SharedImageRef aRef = gSexyAppBase->mResourceManager->GetImage(theName);
	Image* anImage = aRef;
	return anImage;
}

}  // namespace

// 0x4c8970: the root, which is its own parent
XLayout::XLayout() : mParent(this)
{
	Clear();
}

// 0x4c8fa0: a child of theParent
XLayout::XLayout(XLayout* theParent) : mParent(theParent)
{
	Clear();
	mParent = theParent;
}

XLayout::~XLayout()
{
	Clear();
}

// 0x4c9270 (also slot 4): everything back as a new node has it
void XLayout::Clear()
{
	mHorizJust = -1;
	mVertJust = -1;
	mX = 0;
	mY = 0;
	mDebug = false;
	mImage = nullptr;
	mVisible = true;
	mUpdate = true;
	mFont = nullptr;
	mBounds = Rect(0, 0, 0, 0);
	mSrcRect = Rect(0, 0, 0, 0);
	mCel = 0;
	mContent = CONTENT_NONE;
	mText.clear();
	mName.clear();
	mColor = Color(255, 255, 255, 255);
	for (size_t i = 0; i < mChildren.size(); i++)
		delete mChildren[i];
	mChildren.clear();
}

XLayout* XLayout::FindNode(const std::string& theName)
{
	// 0x4c9690
	if (mName == theName)
		return this;
	for (size_t i = 0; i < mChildren.size(); i++) {
		XLayout* aNode = mChildren[i]->FindNode(theName);
		if (aNode != nullptr)
			return aNode;
	}
	return nullptr;
}

int XLayout::ContentWidth() const
{
	// the box's width, or the picture's cel (0x4c8660 is the animation's)
	if (mSrcRect.mWidth >= 1)
		return mSrcRect.mWidth;
	return mImage->GetCelWidth();
}

int XLayout::ContentHeight() const
{
	if (mSrcRect.mHeight >= 1)
		return mSrcRect.mHeight;
	return mImage->GetCelHeight();
}

int XLayout::TextHeight()
{
	// a Graphics of its own, with the node's font: the height of the text word-wrapped to the bounds' width
	Graphics g;
	g.SetFont(mFont);
	return g.GetWordWrappedHeight(mBounds.mWidth, mText, -1, nullptr);
}

void XLayout::Place()
{
	// 0x4cbc30: x, from the horizontal justification (-1: the bounds' x; 1: the far end; else the middle)
	if (mHorizJust == -1) {
		mX = mBounds.mX;
	} else if (mHorizJust == 1) {
		switch (mContent) {
		case CONTENT_NONE:
			mX = mParent->GetWidth() + (mBounds.mX - mBounds.mWidth);
			break;
		case CONTENT_IMAGE:
			mX = (mBounds.mWidth + mBounds.mX) - ContentWidth();
			break;
		case CONTENT_TEXT:
			mX = mBounds.mX;
			break;
		}
	} else {
		switch (mContent) {
		case CONTENT_NONE:
			mX = (mParent->GetWidth() - mBounds.mWidth) / 2 + mBounds.mX;
			break;
		case CONTENT_IMAGE:
			mX = (mBounds.mWidth - ContentWidth()) / 2 + mBounds.mX;
			break;
		case CONTENT_TEXT:
			mX = mBounds.mX;
			break;
		}
	}
	// y, the same way (a text is as high as its words wrapped to the bounds' width take)
	if (mVertJust == -1) {
		mY = mBounds.mY;
	} else if (mVertJust != 0) {
		switch (mContent) {
		case CONTENT_NONE:
			mY = mParent->GetHeight() + (mBounds.mY - mBounds.mHeight);
			break;
		case CONTENT_IMAGE:
			mY = (mBounds.mHeight + mBounds.mY) - ContentHeight();
			break;
		case CONTENT_TEXT:
			mY = (mBounds.mHeight + mBounds.mY) - TextHeight();
			break;
		}
	} else {
		switch (mContent) {
		case CONTENT_NONE:
			mY = (mParent->GetHeight() - mBounds.mHeight) / 2 + mBounds.mY;
			break;
		case CONTENT_IMAGE:
			mY = (mBounds.mHeight - ContentHeight()) / 2 + mBounds.mY;
			break;
		case CONTENT_TEXT:
			mY = (mBounds.mHeight - TextHeight()) / 2 + mBounds.mY;
			break;
		}
	}
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->Place();
}

void XLayout::SetHorizJust(int theJust)
{
	// 0x4c8480
	mHorizJust = theJust;
	Place();
}

void XLayout::SetVertJust(int theJust)
{
	// 0x4c84a0
	mVertJust = theJust;
	Place();
}

void XLayout::SetBoundsX(int theX)
{
	// 0x4c84c0
	mBounds.mX = theX;
	Place();
}

void XLayout::SetBoundsY(int theY)
{
	// 0x4c84d0
	mBounds.mY = theY;
	Place();
}

void XLayout::SetBounds(int theX, int theY, int theWidth, int theHeight)
{
	// 0x4c8530
	mBounds = Rect(theX, theY, theWidth, theHeight);
	Place();
}

void XLayout::SetText(const std::string& theText, int r, int g, int b, int a, Font* theFont)
{
	// 0x4c95e0
	mContent = CONTENT_TEXT;
	mText = theText;
	mColor = Color(r, g, b, a);
	mFont = theFont;
	mImage = nullptr;
	Place();
}

void XLayout::SetImage(Image* theImage, int theCel, const Rect& theSrcRect)
{
	// 0x4c9550 (port: no picture is nothing to draw)
	if (theImage == nullptr) {
		mContent = CONTENT_NONE;
		mImage = nullptr;
		Place();
		return;
	}
	mCel = theCel;
	mContent = CONTENT_IMAGE;
	mImage = theImage;
	mText.clear();
	mFont = nullptr;
	mSrcRect = theSrcRect;
	Place();
}

void XLayout::Align(XLayout* theWho, int theDirection, int theNudge)
{
	// 0x4cbf90: theWho's bounds
	Rect aWho = theWho->mBounds;
	switch (theDirection) {
	case ALIGN_LEFT:
		SetBoundsX(aWho.mX - theNudge);
		break;
	case ALIGN_RIGHT:
		SetBoundsX(aWho.mX + aWho.mWidth + theNudge);
		break;
	case ALIGN_TOP:
		SetBoundsY(aWho.mY - mBounds.mHeight - theNudge);
		break;
	case ALIGN_BOTTOM:
		SetBoundsY(aWho.mY + aWho.mHeight + theNudge);
		break;
	case ALIGN_TOPLEFT:
		Align(theWho, ALIGN_LEFT, theNudge);
		Align(theWho, ALIGN_TOP, theNudge);
		break;
	case ALIGN_TOPRIGHT:
		Align(theWho, ALIGN_RIGHT, theNudge);
		Align(theWho, ALIGN_TOP, theNudge);
		break;
	case ALIGN_BOTTOMLEFT:
		Align(theWho, ALIGN_LEFT, theNudge);
		Align(theWho, ALIGN_BOTTOM, theNudge);
		break;
	case ALIGN_BOTTOMRIGHT:
		Align(theWho, ALIGN_RIGHT, theNudge);
		Align(theWho, ALIGN_BOTTOM, theNudge);
		break;
	case ALIGN_YCENTER:
		SetBoundsY((aWho.mHeight - mBounds.mHeight) / 2 + aWho.mY);
		break;
	case ALIGN_XCENTER:
		SetBoundsX((aWho.mWidth - mBounds.mWidth) / 2 + aWho.mX);
		break;
	}
}

void XLayout::Center(XLayout* theWho1, XLayout* theWho2, bool theHoriz)
{
	// 0x4cc130: in the gap between the two (the one that is further left or up is the first)
	if (theHoriz) {
		XLayout* aFirst = theWho1;
		if (theWho2->GetX() <= theWho1->GetX())
			aFirst = theWho2;
		XLayout* aSecond = theWho2;
		if (theWho2->GetX() <= theWho1->GetX())
			aSecond = theWho1;
		int aLeft = aFirst->GetWidth() + aFirst->GetX();
		SetBoundsX(((aSecond->GetX() - aLeft) - mBounds.mWidth) / 2 + aLeft);
	} else {
		XLayout* aFirst = theWho1;
		if (theWho2->GetY() <= theWho1->GetY())
			aFirst = theWho2;
		XLayout* aSecond = theWho2;
		if (theWho2->GetY() <= theWho1->GetY())
			aSecond = theWho1;
		int aTop = aFirst->GetHeight() + aFirst->GetY();
		SetBoundsY(((aSecond->GetY() - aTop) - mBounds.mHeight) / 2 + aTop);
	}
}

void XLayout::Update()
{
	// 0x4cc270: only the animations move (none are ported)
	if (!mUpdate)
		return;
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->Update();
}

void XLayout::Draw(Graphics* g)
{
	// 0x4cc2f0
	if (!mVisible)
		return;
	if (mDebug) {
		// 0x4cc4c0: the bounds, outlined
		g->PushState();
		g->SetColor(Color(255, 0, 255));
		g->DrawRect(mBounds);
		g->PopState();
	}
	if (mContent == CONTENT_IMAGE) {
		// 0x4cc410
		if (mSrcRect.mWidth < 1) {
			g->DrawImageCel(mImage, mX, mY, mCel);
		} else {
			// the box (a picture of 3 by 3 pieces) from the cel's pieces
			Rect aCel = mImage->mNumCols > 1 ? mImage->GetCelRect(mCel, 0) : mImage->GetCelRect(0, mCel);
			g->DrawImageBox(aCel, Rect(mX, mY, mSrcRect.mWidth, mSrcRect.mHeight), mImage);
		}
	} else if (mContent == CONTENT_TEXT) {
		g->SetFont(mFont);
		g->SetColor(mColor);
		g->WriteWordWrapped(Rect(mX, mY, mBounds.mWidth, mBounds.mHeight), mText, -1, mHorizJust, nullptr, -1);
	}
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->Draw(g);
}

namespace {

// One attribute of an element: its key in capitals, its value as written
struct Attr {
	std::string mKey;
	std::string mValue;
};

std::vector<Attr> GetAttrs(XMLElement& theElement)
{
	std::vector<Attr> aList;
	std::set<std::string> aSeen;
	for (XMLParamMapIteratorList::iterator it = theElement.mAttributeIteratorList.begin();
	     it != theElement.mAttributeIteratorList.end(); ++it) {
		// (the last attribute of an empty element is listed twice by the parser)
		if (!aSeen.insert((*it)->first).second)
			continue;
		Attr anAttr;
		anAttr.mKey = Upper((*it)->first);
		anAttr.mValue = (*it)->second;
		aList.push_back(anAttr);
	}
	return aList;
}

int ParseDirection(const std::string& theValue)
{
	// 0x4c86a0
	std::string aValue = Upper(theValue);
	static const char* const kNames[] = {"LEFT", "RIGHT",  "TOP",     "BOTTOM",      "TOPLEFT",
	                                     "TOPRIGHT", "BOTTOMLEFT", "BOTTOMRIGHT", "YCENTER", "XCENTER"};
	for (int i = 0; i < 10; i++) {
		if (aValue == kNames[i])
			return i;
	}
	return XLayout::ALIGN_UNKNOWN;
}

}  // namespace

bool XLayout::Load(const std::string& theFileName)
{
	// 0x6b3820 and the tags' handlers (below): the elements in the order they come. Each Child is a node of its own
	// that the tags up to its end act on.
	XMLParser aParser;
	if (!aParser.OpenFile(theFileName)) {
		SDL_Log("XLayout: %s: %s", theFileName.c_str(), aParser.GetErrorText().c_str());
		return false;
	}
	XLayout* aCur = this;
	int aDepth = 0;  // Child elements open
	XMLElement anElement;
	for (;;) {
		anElement.mAttributeIteratorList.clear();  // (the parser empties the attributes but not their order)
		if (!aParser.NextElement(&anElement))
			break;
		if (anElement.mType == XMLElement::TYPE_END) {
			if (anElement.mValue == "Child" && aDepth > 0) {
				aCur = aCur->mParent;
				aDepth--;
			}
			continue;
		}
		if (anElement.mType != XMLElement::TYPE_START && anElement.mType != XMLElement::TYPE_ELEMENT)
			continue;
		std::string aTag = Upper(anElement.mValue);
		std::vector<Attr> anAttrs = GetAttrs(anElement);
		if (aTag == "CHILD") {
			XLayout* aChild = new XLayout(aCur);
			aCur->mChildren.push_back(aChild);
			if (anElement.mType == XMLElement::TYPE_START) {
				aCur = aChild;
				aDepth++;
			}
		} else if (aTag == "JUSTIFICATION") {
			// 0x4c9890
			for (size_t i = 0; i < anAttrs.size(); i++) {
				if (anAttrs[i].mKey == "HORIZ")
					aCur->SetHorizJust(atol(anAttrs[i].mValue.c_str()));
				else
					aCur->SetVertJust(atol(anAttrs[i].mValue.c_str()));
			}
		} else if (aTag == "BOUNDS") {
			// 0x4c9a60: x and y from the parent's
			int x = 0, y = 0, w = 0, h = 0;
			for (size_t i = 0; i < anAttrs.size(); i++) {
				const std::string& aKey = anAttrs[i].mKey;
				if (aKey == "X")
					x = aCur->mParent->GetX() + atol(anAttrs[i].mValue.c_str());
				else if (aKey == "Y")
					y = aCur->mParent->GetY() + atol(anAttrs[i].mValue.c_str());
				else if (aKey == "W")
					w = atol(anAttrs[i].mValue.c_str());
				else
					h = atol(anAttrs[i].mValue.c_str());
			}
			aCur->SetBounds(x, y, w, h);
		} else if (aTag == "CONTROL") {
			// 0x4c9d50
			for (size_t i = 0; i < anAttrs.size(); i++) {
				const std::string& aKey = anAttrs[i].mKey;
				if (aKey == "VISIBLE") {
					aCur->SetVisible(IsTrue(anAttrs[i].mValue));
				} else if (aKey == "UPDATE") {
					aCur->SetUpdate(IsTrue(anAttrs[i].mValue));
				} else if (aKey == "DEBUG") {
					aCur->mDebug = IsTrue(anAttrs[i].mValue);
				} else if (aKey == "NAME") {
					if (FindNode(anAttrs[i].mValue) != nullptr) {
						SDL_Log("Error: object of name \"%s\" already defined in XLayout::ParseControl",
						        anAttrs[i].mValue.c_str());
						break;
					}
					aCur->mName = anAttrs[i].mValue;
				}
			}
		} else if (aTag == "DATA") {
			// 0x4ca780: the first of TEXT, IMAGE and ANIM that the tag has decides what it is
			int aKind = 0;
			for (size_t i = 0; i < anAttrs.size() && aKind == 0; i++) {
				if (anAttrs[i].mKey == "TEXT")
					aKind = CONTENT_TEXT;
				else if (anAttrs[i].mKey == "IMAGE")
					aKind = CONTENT_IMAGE;
				else if (anAttrs[i].mKey == "ANIM")
					aKind = CONTENT_ANIM;
			}
			if (aKind == CONTENT_TEXT) {
				// 0x4ca9d0
				std::string aText;
				int r = 0, g = 0, b = 0, a = 255;
				Font* aFont = nullptr;
				for (size_t i = 0; i < anAttrs.size(); i++) {
					const std::string& aKey = anAttrs[i].mKey;
					int aValue = atol(anAttrs[i].mValue.c_str());
					if (aKey == "COLORR")
						r = aValue;
					else if (aKey == "COLORG")
						g = aValue;
					else if (aKey == "COLORB")
						b = aValue;
					else if (aKey == "COLORA")
						a = aValue;
					else if (aKey == "FONT")
						aFont = gSexyAppBase->mResourceManager->GetFont(anAttrs[i].mValue);
					else if (aKey == "TEXT")
						aText = anAttrs[i].mValue;
				}
				aCur->SetText(aText, r, g, b, a, aFont);
			} else if (aKind == CONTENT_IMAGE) {
				// 0x4cad90
				Image* anImage = nullptr;
				int aCel = 0, aBoxW = 0, aBoxH = 0;
				for (size_t i = 0; i < anAttrs.size(); i++) {
					const std::string& aKey = anAttrs[i].mKey;
					if (aKey == "IMAGE")
						anImage = FindImage(anAttrs[i].mValue);
					else if (aKey == "CEL")
						aCel = atol(anAttrs[i].mValue.c_str());
					else if (aKey == "BOXW")
						aBoxW = atol(anAttrs[i].mValue.c_str());
					else if (aKey == "BOXH")
						aBoxH = atol(anAttrs[i].mValue.c_str());
				}
				if (anImage == nullptr) {
					SDL_Log("XLayout: %s: no picture for the Data tag", theFileName.c_str());
				} else {
					int aCount = anImage->mNumCols;
					if (aCount < 2)
						aCount = anImage->mNumRows;
					if (aCel <= aCount)
						aCur->SetImage(anImage, aCel, Rect(0, 0, aBoxW, aBoxH));
				}
			} else if (aKind == CONTENT_ANIM) {
				// port: the animation (0x4cb0f0) is not ported; no layout file has one
				SDL_Log("XLayout: %s: a Data tag with an animation is not ported yet", theFileName.c_str());
				aCur->mContent = CONTENT_NONE;
			} else {
				aCur->mContent = CONTENT_NONE;
			}
			aCur->Place();
		} else if (aTag == "ALIGN") {
			// 0x4ca150
			int aDirection = ALIGN_LEFT;
			int aNudge = 0;
			XLayout* aWho = nullptr;
			for (size_t i = 0; i < anAttrs.size(); i++) {
				const std::string& aKey = anAttrs[i].mKey;
				if (aKey == "DIR")
					aDirection = ParseDirection(anAttrs[i].mValue);
				else if (aKey == "NUDGE")
					aNudge = atol(anAttrs[i].mValue.c_str());
				else if (aKey == "WHO")
					aWho = FindNode(anAttrs[i].mValue);
			}
			if (aWho == nullptr)
				SDL_Log("XLayout: %s: Align names no node that exists", theFileName.c_str());
			else
				aCur->Align(aWho, aDirection, aNudge);
		} else if (aTag == "CENTER") {
			// 0x4ca410
			bool aHoriz = false;
			XLayout* aWho1 = nullptr;
			XLayout* aWho2 = nullptr;
			for (size_t i = 0; i < anAttrs.size(); i++) {
				const std::string& aKey = anAttrs[i].mKey;
				if (aKey == "HORIZ")
					aHoriz = IsTrue(anAttrs[i].mValue);
				else if (aKey == "WHO1")
					aWho1 = FindNode(anAttrs[i].mValue);
				else if (aKey == "WHO2")
					aWho2 = FindNode(anAttrs[i].mValue);
			}
			if (aWho1 == nullptr || aWho2 == nullptr)
				SDL_Log("XLayout: %s: Center names a node that does not exist", theFileName.c_str());
			else
				aCur->Center(aWho1, aWho2, aHoriz);
		}
		// anything else (the Layout tag itself) is no setting
	}
	if (aParser.HasFailed())
		SDL_Log("XLayout: %s: %s", theFileName.c_str(), aParser.GetErrorText().c_str());
	return true;
}
