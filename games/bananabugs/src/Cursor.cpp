#include "Cursor.h"

#include "Anim.h"

using namespace Sexy;

Cursor* Cursor::sCursor = nullptr;

// 0x426e70: the cursor's cels count up through all 32 every 32 frames; its centre is the picture's centre
Cursor::Cursor()
{
	SetParent(nullptr, nullptr);
	mAnim = Anim::Get("images/cursor_anim", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0x20, false);
	SetStateInt(STATE_Z, 2010);
	SetState(STATE_OFFSET_X, mAnim->GetSourceCelWidth() * 0.5f);
	SetState(STATE_OFFSET_Y, mAnim->GetSourceCelHeight() * 0.5f);

	AnimKey* aSpin = new AnimKey();
	aSpin->SetDuration(32);
	aSpin->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 32.0f, 32.0f, 0.0f, nullptr);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aSpin, aSpin);
	AddTrack(aTrack, nullptr);
	Activate(this, false);
	mUpdateLevel = 0;
}

// 0x426ff0
Cursor::~Cursor()
{
}

// 0x427000: the picture sits on the mouse, in window pixels
bool Cursor::Update()
{
	if (!ObjBase::Update())
		return false;
	SetState(STATE_X, sMouseX);
	SetState(STATE_Y, sMouseY);
	return true;
}

void Cursor::Create()
{
	sCursor = new Cursor();
}

void Cursor::Show()
{
	sCursor->Activate(sCursor, false);
}

void Cursor::Hide()
{
	sCursor->Deactivate(sCursor);
}
