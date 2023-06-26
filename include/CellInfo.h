#pragma once

#include "Disease.h"
#include "Season.h"

struct CellInfoType
{
	enum CellType : EnumType
	{
		kIntenseCold = 1 << 0,
		kIntenseHeat = 1 << 1,
		kDessert = 1 << 2,
		kIceland = 1 << 3,
		kAshland = 1 << 4,
		kSwamp = 1 << 5,
		kSnow = 1 << 6,
		kCold = 1 << 7,
		kHeat = 1 << 8,
	};
};

class CellInfo
{
public:
	RE::TESObjectCELL* cell;
	EnumType celltype = CellTypes::kNone;
	Season::Season seasoncalc = Season::Season::kSpring;
	int registeredactors = 0;

	bool IsIntenseCold() {
		return celltype & CellInfoType::kIntenseCold;
	}
	bool IsIntenseHeat() {
		return celltype & CellInfoType::kIntenseHeat;
	}
	bool IsDessert() {
		return celltype & CellInfoType::kDessert;
	}
	bool IsIceland() {
		return celltype & CellInfoType::kIceland;
	}
	bool IsAshland() {
		return celltype & CellInfoType::kAshland;
	}
	bool IsSwamp() {
		return celltype & CellInfoType::kSwamp;
	}
	bool IsSnow() {
		return celltype & CellInfoType::kSwamp;
	}
	bool IsCold() {
		return celltype & CellInfoType::kCold;
	}
	bool IsHeat() {
		return celltype & CellInfoType::kHeat;
	}
	bool IsExtremeCondition() {
		return celltype & CellInfoType::kDessert |
				celltype & CellInfoType::kIceland |
				celltype & CellInfoType::kIntenseCold |
				celltype & CellInfoType::kIntenseHeat |;
	}

	std::vector<Diseases::Disease> GetPossibleInfections();
};
