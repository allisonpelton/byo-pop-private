#ifndef __FEEDINGFRENZY2_VISUALDATA_H__
#define __FEEDINGFRENZY2_VISUALDATA_H__

// A visual's frames at one size, with its model's shapes and points scaled to them (VisualData, 0x8c bytes, vtables
// 0x553e94 and 0x553eb8, its constructor 0x41a410 (feedingfrenzy2)), and a fish's (FishVisualData, 0xa8 bytes,
// vtables 0x550fb4 and 0x550fd8, its constructor 0x414580 (feedingfrenzy2)). The frames are the model's image or its
// animation, each scaled to the size, put through the visual's colour effect and the water's colour, and anchored at
// its centre.

#include "ActorInfo.h"
#include "AnimationData.h"
#include "Geometry.h"
#include "Shape.h"

#include <map>
#include <string>

class VisualData : public engine::AnimationData
{
public:
	VisualData() : mScale(1.0f), mTint(0.0f, 0.0f, 0.0f, 0.0f) {}

	// 0x47b650 (feedingfrenzy2), the info's: the frames of the info's model for the size (a size of 0 or less, or a
	// model of no size, keeps the model's), then finish
	void setup(VisualInfo* theInfo, const std::string& theAnimBase, float theSize);

	// 0x0c (0x4267e0 (feedingfrenzy2)): the frame scaled and coloured, then added
	void addFrame(engine::Surface* theFrame) override;
	// 0x1c (0x404ba0 (feedingfrenzy2)): the shapes and points set for the frames
	virtual void finish();

	// 0x46d780 (feedingfrenzy2): the point of the role, (0, 0) for none
	engine::Vector2f getAnchor(const std::string& theRole) const;
	// The shape of the role, null for none
	Shape* getShape(const std::string& theRole) const;
	float getScale() const { return mScale; }
	VisualInfo* getInfo() const { return mInfo; }

protected:
	// 0x46f180 (feedingfrenzy2): the "visual" shape, the first frame's box around its anchor (a box of (1, 1) at
	// (1, 1) without frames); each of the model's shapes copied, scaled and moved round the anchor; each of its points
	// the same; then the points "body" and "mouth" at the centres of those shapes where the model gives none, else at
	// (0, 0)
	void setupShapes(float theShapeScale);

	engine::Ref<VisualInfo> mInfo;                       // +0x54
	std::map<std::string, engine::Ref<Shape>> mShapes;   // +0x58: by role
	std::map<std::string, engine::Vector2f> mAnchors;    // +0x64: points by role, from the anchor
	float mScale;                                        // +0x70: the size over the model's
	engine::Color mTint;                                 // +0x74: laid over each frame when its alpha is above 0
};

class FishVisualData : public VisualData
{
public:
	FishVisualData() : mInAir(false), mMouth(0.0f, 0.0f) {}

	// 0x484740 (feedingfrenzy2), the info's: the colour and the air kept, the frames set up, the eat, swim and idle
	// runs fitted to the model (0x484100 (feedingfrenzy2)), then the menu's and HUD's pictures
	void setup(FishVisualInfo* theInfo, const std::string& theAnimBase, float theSize, const engine::Color& theTint,
	           bool inAir);

	// 0x18 (0x412f00 (feedingfrenzy2)): a fish out of the air drops its "idle" run before the frames are read
	void loadFrames(const std::string& thePath) override;
	// 0x1c (0x473830 (feedingfrenzy2)): the shapes at the visual's scale, the points "halo", "idea" and "tail" where
	// the model gives none, and the mouth kept
	void finish() override;

	const engine::Vector2f& getMouth() const { return mMouth; }
	FishVisualInfo* getFishInfo() const { return mFishInfo; }
	// The fish's picture on the menus and the stage's end, and its two icons on the HUD; null without a menu picture
	engine::Surface* getMenuPicture() const { return mMenuPicture; }
	engine::Surface* getHudIcon() const { return mHudIcon; }
	engine::Surface* getHudDangerIcon() const { return mHudDangerIcon; }

private:
	// 0x484100 (feedingfrenzy2)
	void fitAnimations();
	// 0x427490 (feedingfrenzy2): the menu's picture "<menu>.png" through the visual's colour effect (rgbBlend's
	// left out); then, when there is one, the HUD's icon of its size, the picture over "<menu>_shadow.png" at its
	// centre, and the danger icon, the shadow, the picture darkened to 0.3 and the shell theme's hudDangerOverlay at
	// (43, 37)
	void setupPictures(const std::string& theMenu);

	engine::Ref<engine::Surface> mMenuPicture;    // +0x84
	engine::Ref<engine::Surface> mHudIcon;        // +0x88
	engine::Ref<engine::Surface> mHudDangerIcon;  // +0x8c
	engine::Ref<FishVisualInfo> mFishInfo;  // +0x90
	bool mInAir;                            // +0x94
	engine::Vector2f mMouth;                // +0x98: the "mouth" point
};

#endif
