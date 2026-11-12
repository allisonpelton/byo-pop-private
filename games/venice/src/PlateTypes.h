// The kinds of plate (the names the data files give them are in the shape names): each places its slots in its own pattern and
// answers a jewel's hit in its own way. The round, semicircle, T, square and spinner plates turn when a jewel hits them, the
// stick, triangle, A, step and M plates wobble, the tosser plate throws its jewels away, and the key plate pulls the baskets in.

#ifndef VENICE_PLATETYPES_H
#define VENICE_PLATETYPES_H

#include "Plate.h"

class RoundPlate : public Plate
{
public:
	RoundPlate() {} // 0x4097f0
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x414e80
	void SetUp() override; // 0x414ec0
	void OnSlotFilled() override { BoostSpin(); } // 0x414e40
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x414f00
};

class SemicirclePlate : public Plate
{
public:
	float mSpin = 0; // 0x178: the speed the plate swings at
	SemicirclePlate() {} // 0x409810
	bool Update() override; // 0x415430
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x415310
	void SetUp() override; // 0x415350
	void OnSlotFilled() override { BoostSpin(); } // 0x414e40
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x415390
};

class APlate : public Plate
{
public:
	APlate() {} // 0x409830
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x4155b0
	void SetUp() override; // 0x4154f0
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x415520
};

class XPlate : public Plate
{
public:
	XPlate() {} // 0x409850
	void SetUp() override; // 0x4156a0
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x4156d0
};

class StickPlate : public Plate
{
public:
	StickPlate() {} // 0x409870
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x414fc0
	void SetUp() override; // 0x415090
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x4150c0
};

class BlockPlate : public Plate
{
public:
	BlockPlate() {} // 0x409890
	void PopEffect() override {} // 0x403510: this plate pops without a show
	void SetUp() override; // 0x415ad0
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x415b00
};

class TrianglePlate : public Plate
{
public:
	TrianglePlate() {} // 0x4098b0
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x415160
	void SetUp() override; // 0x415250
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x415280
};

class TPlate : public Plate
{
public:
	float mSpin = 0; // 0x178: the speed the plate swings at
	TPlate() {} // 0x4098d0
	bool Update() override; // 0x415a20
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x4159e0
	void SetUp() override; // 0x415920
	void OnSlotFilled() override { BoostSpin(); } // 0x414e40
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x415960
};

class TosserPlate : public Plate
{
public:
	TosserPlate() {} // 0x4098f0
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x415d80
	void SetUp() override; // 0x415c30
	void OnSlotFilled() override; // 0x415db0: every slot throws its jewel away and is gone
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x415c60
};

class KeyPlate : public Plate
{
public:
	KeyPlate() {} // 0x409910
	void PopEffect() override; // 0x415bf0
	void SetUp() override; // 0x415b70
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x415ba0
};

class SpinnerPlate : public Plate
{
public:
	SpinnerPlate() {} // 0x409930
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x415e30
	void SetUp() override; // 0x415e70
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x415eb0
};

class StepPlate : public Plate
{
public:
	StepPlate() {} // 0x409950
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x415f20
	void SetUp() override; // 0x416010
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x416040
};

class SquarePlate : public Plate
{
public:
	SquarePlate() {} // 0x409970
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x414e80
	void SetUp() override; // 0x4160e0
	void OnSlotFilled() override { BoostSpin(); } // 0x414e40
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x416120
};

class MPlate : public Plate
{
public:
	MPlate() {} // 0x409990
	void OnJewelHit(Slot* theSlot, Jewel* theJewel) override; // 0x416280
	void SetUp() override; // 0x416370
	void PositionSlot(Slot* theSlot, int theIndex) override; // 0x4163a0
};

#endif
