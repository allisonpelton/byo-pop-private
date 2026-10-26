// XWidget::XLayout (vtable 0x7b5834, code 0x4c8420-0x4cc4c0 and its loader 0x6b2d40-0x6b4f30; the Hall of Fame's
// +0x8c, the stats book's and the mini-games' menu's): a tree of named places, read from an XML file
// (data/hof_*.xml, data/layout_*.xml). A node has bounds (x, y, width, height), a justification, a thing it
// draws (a picture, a picture stretched over a box, or a word-wrapped text) and children; Align and Center put it
// next to its siblings. The screens ask for a node by its name (FindNode) and place their buttons and drawings
// by its resolved x and y (GetX, GetY).
//
//   <Layout>  Justification horiz vert      -1: at the bounds' x or y, 0: centred in the bounds, 1: at the far end
//             Bounds x y w h                x and y are added to the parent's resolved x and y
//             Control visible update debug name
//             Data image cel boxw boxh      a picture (a box of boxw by boxh when boxw is given), or
//             Data text font colorr colorg colorb colora     a text, or (anim ...) an animation
//             Align dir who nudge           dir: Left Right Top Bottom TopLeft TopRight BottomLeft BottomRight
//                                           YCenter XCenter (against the node named who)
//             Center horiz who1 who2        between two nodes
//             Child ... /Child              a node of its own
//
// port: the original read the XML with its own loader (XWidget's, a table of handlers for each tag) and found
// the pictures and fonts through three function pointers the screen gave it (+0x114, +0x118, +0x11c: a
// resource's name, its id, the picture or font of an id); this reads it with the framework's XMLParser in
// document order (each setting acts as it is read, as the original's did) and looks the names up in the app's
// resource manager. The animation kind of Data (ANIM, 0x4cb0f0, a PopAnim-like strip with a delay, a loop count
// and more) is not ported: no layout file of the game uses it.
#pragma once

#include <string>
#include <vector>

#include "Color.h"
#include "Rect.h"

namespace Sexy {

class Font;
class Graphics;
class Image;

class XLayout {
public:
	// what a node draws (+0x120)
	enum Content {
		CONTENT_NONE = 0,
		CONTENT_ANIM = 1,   // port: not ported
		CONTENT_IMAGE = 2,  // a picture's cel, or a box of the picture
		CONTENT_TEXT = 3,
	};
	// Align's directions (0x4c86a0), also the order of the jump table in Align (0x4cbf90)
	enum Direction {
		ALIGN_LEFT = 0,
		ALIGN_RIGHT = 1,
		ALIGN_TOP = 2,
		ALIGN_BOTTOM = 3,
		ALIGN_TOPLEFT = 4,
		ALIGN_TOPRIGHT = 5,
		ALIGN_BOTTOMLEFT = 6,
		ALIGN_BOTTOMRIGHT = 7,
		ALIGN_YCENTER = 8,
		ALIGN_XCENTER = 9,
		ALIGN_UNKNOWN = 10,
	};

	XLayout* mParent;                  // +0xf0 (the root's is itself)
	std::vector<XLayout*> mChildren;   // +0xd4
	std::string mName;                 // +0xfc
	Rect mBounds;                      // +0x60
	Rect mSrcRect;                     // +0x70: (0, 0, boxw, boxh) of a picture, 0 for a plain cel
	Font* mFont;                       // +0x80
	std::string mText;                 // +0x84
	Image* mImage;                     // +0xbc
	Color mColor;                      // +0xc0: the text's
	int mContent;                      // +0x120
	int mHorizJust;                    // +0x124
	int mVertJust;                     // +0x128
	int mCel;                          // +0x12c
	int mX;                            // +0x130: where it is, from the bounds and the justification
	int mY;                            // +0x134
	bool mVisible;                     // +0x138
	bool mUpdate;                      // +0x139
	bool mDebug;                       // +0x13b: draws its bounds in magenta

	XLayout();                         // 0x4c8970
	explicit XLayout(XLayout* theParent);  // 0x4c8fa0
	virtual ~XLayout();                // 0x4c9160

	// 0x6b3820: reads theFileName into this node (as the root); a file that does not open leaves it as it is
	bool Load(const std::string& theFileName);
	// 0x4c9270: back to an empty node (the children go)
	void Clear();
	// 0x4c9690: the node of that name, this one or below it; NULL when there is none
	XLayout* FindNode(const std::string& theName);

	int GetX() const { return mX; }                    // slot 29, 0x4c85d0
	int GetY() const { return mY; }                    // slot 30, 0x4c85e0
	int GetWidth() const { return mBounds.mWidth; }    // slot 31, 0x4c85f0
	int GetHeight() const { return mBounds.mHeight; }  // slot 32, 0x4c8600

	void SetHorizJust(int theJust);                    // slot 14, 0x4c8480
	void SetVertJust(int theJust);                     // slot 15, 0x4c84a0
	void SetBoundsX(int theX);                         // slot 16, 0x4c84c0
	void SetBoundsY(int theY);                         // slot 17, 0x4c84d0
	void SetBoundsHeight(int theHeight);               // slot 20, 0x4c8520
	void SetBounds(int theX, int theY, int theWidth, int theHeight);  // slot 22, 0x4c8530
	// slots 13 and 12 (0x4c8460, 0x4c9670): the part of the picture a box is drawn from
	Rect GetSrcRect() const { return mSrcRect; }
	void SetSrcRect(const Rect& theSrcRect) { mSrcRect = theSrcRect; }
	void SetVisible(bool theVisible) { mVisible = theVisible; }   // slot 23, 0x4c8590
	void SetUpdate(bool theUpdate) { mUpdate = theUpdate; }       // slot 24, 0x4c85a0
	// slot 7, 0x4c95e0: a text drawn word-wrapped in the bounds
	void SetText(const std::string& theText, int r, int g, int b, int a, Font* theFont);
	// slot 10, 0x4c9550: a picture's cel, or (theSrcRect with a size) the picture stretched over a box of that size
	void SetImage(Image* theImage, int theCel, const Rect& theSrcRect);
	// slot 25, 0x4cbf90: put against theWho's bounds
	void Align(XLayout* theWho, int theDirection, int theNudge);
	// slot 26, 0x4cc130: centred between theWho1 and theWho2 (across when theHoriz, else down)
	void Center(XLayout* theWho1, XLayout* theWho2, bool theHoriz);

	void Update();           // slot 6, 0x4cc270: the animations (none are ported)
	void Draw(Graphics* g);  // slot 5, 0x4cc2f0: this node's, then the children's

private:
	// 0x4cbc30: mX and mY from the bounds, the justification and what it draws (and the children's)
	void Place();
	int ContentWidth() const;   // the picture's cel width, or its box's
	int ContentHeight() const;
	int TextHeight();

	XLayout(const XLayout&);
	XLayout& operator=(const XLayout&);
};

}  // namespace Sexy
