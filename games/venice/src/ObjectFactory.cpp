// The type names of the data files and the classes they make (0x4381a0, and the table of the game's own classes at 0x4076b0).

#include "ObjectFactory.h"

#include "BonusBasket.h"
#include "Cloud.h"
#include "Cursor.h"
#include "Emitter.h"
#include "EndlessBonus.h"
#include "Firework.h"
#include "InputBox.h"
#include "Layer.h"
#include "LevelLocker.h"
#include "LevelSelect.h"
#include "Obstacle.h"
#include "Panel.h"
#include "Pigeon.h"
#include "PlateTypes.h"
#include "Powerup.h"
#include "SceneObject.h"
#include "Shooter.h"
#include "ShooterObjects.h"
#include "Slot.h"
#include "TextObject.h"
#include "Tooltip.h"
#include "Tutorial.h"
#include "Turret.h"
#include "Warp.h"
#include "Widgets.h"

#include <cstring>

namespace
{
	struct ClassEntry
	{
		const char* mName;
		SceneObject* (*mCreate)();
	};

	template <class T> SceneObject* Create()
	{
		return new T();
	}

	// the order of the original's lookups: the engine's classes, then the game's
	const ClassEntry kClasses[] = {
		{"text", Create<TextObject>},
		{"input_box", Create<InputBox>},
		{"slider", Create<Slider>},
		{"checkbox", Create<Checkbox>},
		{"user_list_box", Create<UserListBox>},
		{"layer", Create<Layer>},
		{"cursor", Create<Cursor>},
		{"user_data_display", Create<UserDataDisplay>},
		{"emitter", Create<Emitter>},
		{"round_plate", Create<RoundPlate>},
		{"semicircle_plate", Create<SemicirclePlate>},
		{"a_plate", Create<APlate>},
		{"x_plate", Create<XPlate>},
		{"stick_plate", Create<StickPlate>},
		{"block_plate", Create<BlockPlate>},
		{"triangle_plate", Create<TrianglePlate>},
		{"t_plate", Create<TPlate>},
		{"tosser_plate", Create<TosserPlate>},
		{"key_plate", Create<KeyPlate>},
		{"spinner_plate", Create<SpinnerPlate>},
		{"step_plate", Create<StepPlate>},
		{"square_plate", Create<SquarePlate>},
		{"m_plate", Create<MPlate>},
		{"slot", Create<Slot>},
		{"turret", Create<Turret>},
		{"warp", Create<Warp>},
		{"rect_bumper", Create<RectBumper>},
		{"round_bumper", Create<RoundBumper>},
		{"tri_bumper", Create<TriBumper>},
		{"pigeon", Create<Pigeon>},
		{"cloud", Create<Cloud>},
		{"firework", Create<Firework>},
		{"shooter", Create<Shooter>},
		{"shooter_cursor", Create<ShooterCursor>},
		{"shooter_path", Create<ShooterPath>},
		{"scroll_point", Create<ScrollPoint>},
		{"powerup", Create<Powerup>},
		{"bonus_basket", Create<BonusBasket>},
		{"endless_bonus", Create<EndlessBonus>},
		{"panel", Create<Panel>},
		{"endless_panel", Create<EndlessPanel>},
		{"progress_bar", Create<ProgressBar>},
		{"level_select", Create<LevelSelect>},
		{"level_scroller", Create<LevelScroller>},
		{"level_locker", Create<LevelLocker>},
		{"tooltip", Create<Tooltip>},
		{"tutorial", Create<Tutorial>},
	};
}

SceneObject* NewObjectByTypeName(const char* theTypeName)
{
	for (const ClassEntry& anEntry : kClasses) {
		if (strcmp(theTypeName, anEntry.mName) == 0)
			return anEntry.mCreate();
	}
	return new SceneObject();
}
