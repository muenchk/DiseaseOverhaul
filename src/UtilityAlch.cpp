#include "UtilityAlch.h"

std::string UtilityAlch::ToString(Diseases::Disease value)
{
	switch (value) {
	case Diseases::kAshChancre:
		return "Ash Chancre";
	case Diseases::kAshWoeBlight:
		return "Ash Woe Blight";
	case Diseases::kAstralVapors:
		return "Astral Vapors";
	case Diseases::kAtaxia:
		return "Ataxia";
	case Diseases::kBlackHeartBlight:
		return "Black-Heart-Blight";
	case Diseases::kBloodLung:
		return "Blood-Lung";
	case Diseases::kBloodRot:
		return "Blood-Rot";
	case Diseases::kBoneBreakFever:
		return "Bone Break Fever";
	case Diseases::kBrainFever:
		return "Brain Fever";
	case Diseases::kBrainRot:
		return "Brain Rot";
	case Diseases::kBrownRot:
		return "Brown Rot";
	case Diseases::kCalironsCurse:
		return "Caliron's Curse";
	case Diseases::kChantraxBlight:
		return "Chantrax Blight";
	case Diseases::kChills:
		return "Chills";
	case Diseases::kCholera:
		return "Cholera";
	case Diseases::kChrondiasis:
		return "Chrondiasis";
	case Diseases::kCollywobbles:
		return "Collywobbles";
	case Diseases::kDampworm:
		return "Campworm";
	case Diseases::kDroops:
		return "Droops";
	case Diseases::kFeeblelimb:
		return "Feeblelimb";
	case Diseases::kGreenspore:
		return "Greenspore";
	case Diseases::kHelljoint:
		return "Helljoint";
	case Diseases::kLeprosy:
		return "Leprosy";
	case Diseases::kPlague:
		return "Plague";
	case Diseases::kRattles:
		return "Rattles";
	case Diseases::kRedDeath:
		return "Red Death";
	case Diseases::kRedRage:
		return "Red Rage";
	case Diseases::kRockjoint:
		return "Rockjoint";
	case Diseases::kRustchancre:
		return "Rust chancre";
	case Diseases::kSerpigniousDementia:
		return "Serpignious Dementia";
	case Diseases::kShakes:
		return "Shakes";
	case Diseases::kStomachRot:
		return "Stomach Rot";
	case Diseases::kSwampFever:
		return "Swamp Fever";
	case Diseases::kSwampRot:
		return "Swamp Rot";
	case Diseases::kTyphoidFever:
		return "Typhoid Fever";
	case Diseases::kWitbane:
		return "Witbane";
	case Diseases::kWitchesPox:
		return "Witches' Pox";
	case Diseases::kWither:
		return "Wither";
	case Diseases::kWitlessPox:
		return "Witless Pox";
	case Diseases::kWizardFever:
		return "Wizard Fever";
	case Diseases::kWoundRot:
		return "Wound Rot";
	case Diseases::kYellowFever:
		return "Yellow Fever";
	case Diseases::kYellowTick:
		return "Yellow Tick";
	default:
		return "Unknown";
	}
}
