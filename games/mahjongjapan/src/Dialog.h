#ifndef __MAHJONGJAPAN_DIALOG_H__
#define __MAHJONGJAPAN_DIALOG_H__

// The game's dialogs (0x403420..0x404ef0). Every dialog is a layer (0x404bd0, vtable 0x51c888) whose first object is a
// frame (0x403ab0, vtable 0x51c830): a scroll drawn from corners and edge pieces (images 0x114-0x11a) around a
// stretched background (0x118), a title and a subtitle, up to ten texts that can appear late and fade in, and a
// message shown for some seconds. Its buttons (0x4037b0, vtable 0x51c7b0) are either images of their own or a strip
// of image 0x11f stretched to a width. YesNoBox (0x4034c0, vtable 0x51c784) asks yes or no and tells a listener.

#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
}

namespace mahjongjapan
{

// The answer of a YesNoBox (the n=2 vtables of the layers that implement it): true lets the box close
class DialogListener
{
public:
	virtual bool OnYes(int theId) = 0;  // +0x00
	virtual bool OnNo(int theId) = 0;   // +0x04
};

// 0x4037b0, 0x403830
class DialogButton : public jb::Button
{
public:
	int mMiddles = 0;  // +0xe8 the 8-pixel middle pieces of the strip
	bool mImage;       // +0xec an image of its own (0x129) instead of the strip

	// 0x4037b0: image 0x129 sending theCmd
	explicit DialogButton(int theCmd);
	// 0x403830: the strip at least theWidth wide, sending theCmd
	DialogButton(int theCmd, int theWidth);
	// 0x403810, 0x403880
	~DialogButton() override {}

	void DrawNormal() override;    // 0x403900
	void DrawHover() override;     // 0x403930
	void DrawPressed() override;   // 0x403960
	void DrawDisabled() override;  // 0x403990
	void OnInit() override {}      // 0x429700

	// 0x403890: a string of Resource.dll in font 0xa0
	void SetText(int theStringId);
	// 0x4039c0: the strip's frame at theLeft
	void DrawStrip(int theLeft);
};

// 0x403ab0
class DialogFrame : public jb::Object
{
public:
	// 0x50 bytes at +0xa0
	struct Text
	{
		std::string mText;    // +0x00
		bool mAltFont;        // +0x08 font 0x133 instead of 0x122
		int mKind;            // +0x0c 0 lines centred, 1 lines
		int mX;               // +0x10
		int mY;               // +0x14
	};

	std::string mTitle;            // +0x90
	Text mTexts[10];               // +0xa0
	int mTextCount = 0;            // +0x3c0
	jb::Font* mSubtitleFont = nullptr;  // +0x3c8 (0x11c)
	jb::Font* mTitleFont = nullptr;     // +0x3cc (0x11e)
	jb::Font* mTextFont = nullptr;      // +0x3d0 (0x122)
	jb::Font* mAltFont = nullptr;       // +0x3d4 (0x133)
	bool mLaidOut = false;         // +0x3d8
	int mWidth = 0;                // +0x3dc
	int mHeight = 0;               // +0x3e0
	int mLeft = 0;                 // +0x3e4 the background's place
	int mTop = 0;                  // +0x3e8
	int mRight = 0;                // +0x3ec
	int mBottom = 0;               // +0x3f0
	int mScaleX = 0;               // +0x3f4 the background's (256x256) scale
	int mScaleY = 0;               // +0x3f8
	int mCorners[4] = {-1};        // +0x3fc top left, top right, bottom left, bottom right: 0 large, 1 small
	int* mEdges[4] = {};           // +0x40c left, right, top, bottom: piece kinds 1 short, 2 middle, 3 long
	int mEdgeCounts[4] = {};       // +0x41c
	int mMessageX;                 // +0x42c
	int mMessageY;                 // +0x430
	std::string mMessage;          // +0x434
	int mMessageSeconds = 0;       // +0x43c
	int mMessageStart;             // +0x440
	int mClickCmd = -1;            // +0x444 a click anywhere sends it to the layer
	bool mAlwaysUpdate = false;    // +0x448

	// 0x403ab0
	DialogFrame();
	// 0x403b90, 0x403bc0
	~DialogFrame() override;

	void Draw() override;                                            // 0x403cc0
	void Init() override;                                            // 0x4045e0
	bool OnLButtonDown(int theFlags, int theX, int theY) override;   // 0x404b60
	bool Update() override;                                          // 0x404650

	// 0x403ca0
	void SetAlwaysUpdate(bool theOn);
	// 0x404720
	void SetCorners(int theTopLeft, int theTopRight, int theBottomLeft, int theBottomRight);
	// 0x404750: lists ending in 0
	void SetEdges(const int* theLeft, const int* theRight, const int* theTop, const int* theBottom);
	// 0x4047c0
	void SetTitle(const char* theTitle) { mTitle = theTitle; }
	// 0x4047e0
	void AddText(const char* theText, int theKind, int theX, int theY, bool theAltFont);
	// 0x4048e0
	void ShowMessage(int theX, int theY, const char* theText, int theSeconds);
	// 0x404990
	int GetTextWidth(const char* theText);
	// 0x4049b0: the size from the corners and the edges
	void Layout();
	// 0x404b80
	void SetClickCmd(int theCmd);
	// 0x404ba0
	void GetSize(int* theSize);
};

// 0x404930: a copy of a list ending in 0; its length
int CopyEdgeList(int*& theList, const int* theSource);

// 0x404bd0
class Dialog : public jb::Layer
{
public:
	DialogFrame* mFrame = nullptr;  // +0x50

	// 0x404bd0
	Dialog(int theX, int theY);
	// 0x404c00, 0x404c20
	~Dialog() override {}

	void Load() override;  // 0x404c40
	virtual void Create() = 0;  // +0x28

	// 0x404c30
	void SetAlwaysUpdate(bool theOn) { mFrame->SetAlwaysUpdate(theOn); }
	// 0x404cb0
	void SetCorners(int theTopLeft, int theTopRight, int theBottomLeft, int theBottomRight)
	{
		mFrame->SetCorners(theTopLeft, theTopRight, theBottomLeft, theBottomRight);
	}
	// 0x404cd0
	void SetEdges(const int* theLeft, const int* theRight, const int* theTop, const int* theBottom)
	{
		mFrame->SetEdges(theLeft, theRight, theTop, theBottom);
	}
	// 0x404cf0: a string of Resource.dll
	void SetTitle(int theStringId);
	// 0x404d60
	void ClearTexts() { mFrame->mTextCount = 0; }
	// 0x404d70: a string of Resource.dll
	void AddText(int theStringId, int theKind, int theX, int theY, bool theAltFont);
	// 0x404e00
	void AddText(const char* theText, int theKind, int theX, int theY, bool theAltFont)
	{
		mFrame->AddText(theText, theKind, theX, theY, theAltFont);
	}
	// 0x404e30: a string of Resource.dll
	void ShowMessage(int theX, int theY, int theStringId, int theSeconds);
	// 0x404eb0
	int GetTextWidth(const char* theText) { return mFrame->GetTextWidth(theText); }
	// 0x404ec0
	void GetSize(int* theSize) { mFrame->GetSize(theSize); }
	// 0x404ed0
	void SendCommand(int theCmd) { OnCommand(theCmd); }
	// 0x404ee0
	void SetClickCmd(int theCmd) { mFrame->SetClickCmd(theCmd); }
};

// 0x4034c0: a question with a yes button (command 1) and a no button (command 0)
class YesNoBox : public Dialog
{
public:
	DialogListener* mListener;  // +0x54
	int mSize;                  // +0x58 0 small, 1 middle, 2 large
	int mId;                    // +0x5c given to the listener
	int mYesText;               // +0x60
	int mNoText;                // +0x64
	bool mImageButtons;         // +0x68
	int mYesWidth;              // +0x6c
	int mNoWidth;               // +0x70

	// 0x4034c0
	YesNoBox(DialogListener* theListener, int theSize, int theId, int theYesText, int theNoText, bool theImageButtons,
		int theYesWidth, int theNoWidth);
	// 0x403520, 0x403540
	~YesNoBox() override {}

	bool OnCommand(int theCmd) override;  // 0x403750
	void Create() override;               // 0x403550
};

// 0x403420: the save slot's picture and text in the common data folder
void GetSlotFiles(std::string& theBmp, std::string& theTxt, int theSlot);

} // namespace mahjongjapan

#endif
