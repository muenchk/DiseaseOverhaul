#include "Random.h"
#include "Threading.h"
#include "UtilityAlch.h"

#include <omp.h>

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
	case Diseases::kSanguinareVampirism:
		return "Sanguinare Vampirism";
	default:
		return "Unknown";
	}
}

std::string UtilityAlch::ToString(std::shared_ptr<Disease>& disease)
{
	return std::string("[") + typeid(Disease).name() + "<" + disease->_name + "><Disease:" + ToString(disease->_disease) + "><Type:" + std::to_string(static_cast<int>(disease->_type)) + "><Stages" + std::to_string(disease->_numstages) + "><StageInfec:" + disease->_stageInfection->_specifier + "><StageIncub:" + disease->_stages[0]->_specifier + "><Stage1:" + disease->_stages[1]->_specifier + "><Stage2:" + disease->_stages[2]->_specifier + "><Stage3:" + disease->_stages[3]->_specifier + "><Stage4:" + disease->_stages[4]->_specifier + "><ProgressionPoints:" + std::to_string(disease->_baseProgressionPoints) + "><InfectionReduction:" + std::to_string(disease->_baseInfectionReductionPoints) + "><InfectionChance:" + std::to_string(disease->_baseInfectionChance) + "><Modifiers" + std::to_string(disease->_validModifiers) + "><ImmunityTime" + std::to_string(disease->immunityTime) + "><Effect:" + PrintForm(disease->endeffect) + "><EndEvents" + std::to_string(disease->endevents) + ">]";
}

std::string UtilityAlch::ToString(std::shared_ptr<DiseaseStage>& stage)
{
	return std::string("[") + typeid(DiseaseStage).name() + "<" + stage->_specifier + "><AdvancementTreshold:" + std::to_string(stage->_advancementThreshold) + "><AdvancementTime:" + std::to_string(stage->_advancementTime) + "><Infectivity:" + std::to_string(static_cast<int>(stage->_infectivity)) + "><Effect:" + PrintForm(stage->effect) + ">]";
 }

bool UtilityAlch::CalcChance(float chance)
{
	if (chance == 0)
		return false;
	return Random::rand1000(Random::rand) < (chance * 10);
}

std::vector<std::shared_ptr<ActorInfo>> UtilityAlch::GetInfectedActors(std::vector<std::shared_ptr<ActorInfo>> actors)
{
	std::vector<std::shared_ptr<ActorInfo>> infected;
	for (int i = 0; i < actors.size(); i++) {
		if (actors[i]->IsInfected())
			infected.push_back(actors[i]);
	}
	return infected;
}

std::vector<std::shared_ptr<ActorInfo>> UtilityAlch::GetProgressingActors(std::vector<std::shared_ptr<ActorInfo>> actors)
{
	std::vector<std::shared_ptr<ActorInfo>> infected;
	for (int i = 0; i < actors.size(); i++) {
		if (actors[i]->IsInfectedProgressing())
			infected.push_back(actors[i]);
	}
	return infected;
}

std::vector<std::shared_ptr<ActorInfo>> UtilityAlch::GetProgressingActors(std::vector<std::shared_ptr<ActorInfo>> actors, Diseases::Disease disease)
{
	std::vector<std::shared_ptr<ActorInfo>> infected;
	for (int i = 0; i < actors.size(); i++) {
		if (actors[i]->IsInfectedProgressing(disease)) {
			infected.push_back(actors[i]);
		}
	}
	return infected;
}

std::vector<std::tuple<std::shared_ptr<ActorInfo>, std::shared_ptr<ActorInfo>, float /*distance*/>> UtilityAlch::GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors)
{
	std::vector<std::tuple<std::shared_ptr<ActorInfo>, std::shared_ptr<ActorInfo>, float /*distance*/>> distances;
	float distance = 0.0f;
	RE::TESObjectCELL* cell = nullptr;
	RE::TESObjectCELL* cell2 = nullptr;
	// iterate over actors
	for (int i = 0; i < actors.size(); i++) {
		// iterate over all actors that the first one does not have an entry with so far
		for (int c = i + 1; c < actors.size(); c++) {
			// skip if actors are the same
			if (actors[i]->GetFormID() == actors[c]->GetFormID())
				continue;
			// calc distance

			// if actors in same worldspace -> calc distance directly
			// if actors in exterior / interior -> set to max value

			// if cells have same interior cell flag
			cell = actors[i]->GetParentCell();
			cell2 = actors[c]->GetParentCell();
			if (cell != nullptr && cell2 != nullptr) {
				auto cellflags = actors[i]->GetParentCellFlags();
				if ((cellflags & RE::TESObjectCELL::Flag::kIsInteriorCell) == (actors[c]->GetParentCellFlags() & RE::TESObjectCELL::Flag::kIsInteriorCell)) {
					// if it actually is an interior cell, check whether its the same
					if (cell->IsInteriorCell()) {
						if (cell->GetFormID() == cell2->GetFormID()) {
							distance = sqrtf(actors[i]->GetPosition().GetSquaredDistance(actors[c]->GetPosition()));
						}
						// if its not the same set ifinity
						else {
							distance = FLT_MAX;
						}
					} else {
						// is exterior cell: check wordspace
						if (actors[i]->GetWorldspaceID() == actors[c]->GetWorldspaceID()) {
							// same worldspace
							distance = sqrtf(actors[i]->GetPosition().GetSquaredDistance(actors[c]->GetPosition()));
						} else {
							// different worlspace, set to infinity
							distance = FLT_MAX;
						}
					}
				} else {
					// different cell types, set to infinity
					distance = FLT_MAX;
				}
				// add to vector
				distances.push_back({ actors[i], actors[c], distance });
			}
		}
	}
	return distances;
}

std::vector<std::vector<std::tuple<int /*idx in actors2*/, float /*distance*/>>> UtilityAlch::GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2)
{
	std::vector<std::vector<std::tuple<int, float /*distance*/>>> distances;
	float distance = 0.0f;
	RE::TESObjectCELL* cell = nullptr;
	RE::TESObjectCELL* cell2 = nullptr;
	// iterate over actors
	for (int i = 0; i < actors1.size(); i++) {
		// create empty new vector
		distances.push_back(std::vector<std::tuple<int, float /*distance*/>>{});
		// iterate over all actors in the second list
		for (int c = 0; c < actors2.size(); c++) {
			// skip if actors are the same
			if (actors1[i]->GetFormID() == actors2[c]->GetFormID())
				continue;
			// calc distance

			// if actors in same worldspace -> calc distance directly
			// if actors in exterior / interior -> set to max value

			// if cells have same interior cell flag
			cell = actors1[i]->GetParentCell();
			cell2 = actors2[c]->GetParentCell();
			if (cell != nullptr && cell2 != nullptr) {
				auto cellflags = actors1[i]->GetParentCellFlags();
				if ((cellflags & RE::TESObjectCELL::Flag::kIsInteriorCell) == (actors2[c]->GetParentCellFlags() & RE::TESObjectCELL::Flag::kIsInteriorCell)) {
					// if it actually is an interior cell, check whether its the same
					if (cell->IsInteriorCell()) {
						if (cell->GetFormID() == cell2->GetFormID()) {
							distance = sqrtf(actors1[i]->GetPosition().GetSquaredDistance(actors2[c]->GetPosition()));
						}
						// if its not the same set ifinity
						else {
							distance = FLT_MAX;
						}
					} else {
						// is exterior cell: check wordspace
						if (actors1[i]->GetWorldspaceID() == actors2[c]->GetWorldspaceID()) {
							// same worldspace
							distance = sqrtf(actors1[i]->GetPosition().GetSquaredDistance(actors2[c]->GetPosition()));
						} else {
							// different worlspace, set to infinity
							distance = FLT_MAX;
						}
					}
				} else {
					// different cell types, set to infinity
					distance = FLT_MAX;
				}
				// add to vector if actors aren't the same
				if (actors1[i]->GetFormID() != actors2[c]->GetFormID())
					distances[i].push_back({ c, distance });
			}
		}
	}
	return distances;
}

std::vector<std::vector<std::tuple<int /*idx in actors2*/, float /*distance*/>>> UtilityAlch::GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2, float distancethreshold)
{
	std::vector<std::vector<std::tuple<int, float /*distance*/>>> distances;
	float distance = 0.0f;
	RE::TESObjectCELL* cell = nullptr;
	RE::TESObjectCELL* cell2 = nullptr;
	// iterate over actors
	for (int i = 0; i < actors1.size(); i++) {
		// create empty new vector
		distances.push_back(std::vector<std::tuple<int, float /*distance*/>>{});
		// iterate over all actors in the second list
		for (int c = 0; c < actors2.size(); c++) {
			// skip if actors are the same
			if (actors1[i]->GetFormID() == actors2[c]->GetFormID())
				continue;
			// calc distance

			// if actors in same worldspace -> calc distance directly
			// if actors in exterior / interior -> set to max value

			// if cells have same interior cell flag
			cell = actors1[i]->GetParentCell();
			cell2 = actors2[c]->GetParentCell();
			if (cell != nullptr && cell2 != nullptr) {
				auto cellflags = actors1[i]->GetParentCellFlags();
				if ((cellflags & RE::TESObjectCELL::Flag::kIsInteriorCell) == (actors2[c]->GetParentCellFlags() & RE::TESObjectCELL::Flag::kIsInteriorCell)) {
					// if it actually is an interior cell, check whether its the same
					if (cell->IsInteriorCell()) {
						if (cell->GetFormID() == cell2->GetFormID()) {
							distance = sqrtf(actors1[i]->GetPosition().GetSquaredDistance(actors2[c]->GetPosition()));
						}
						// if its not the same set ifinity
						else {
							distance = FLT_MAX;
						}
					} else {
						// is exterior cell: check wordspace
						if (actors1[i]->GetWorldspaceID() == actors2[c]->GetWorldspaceID()) {
							// same worldspace
							distance = sqrtf(actors1[i]->GetPosition().GetSquaredDistance(actors2[c]->GetPosition()));
						} else {
							// different worlspace, set to infinity
							distance = FLT_MAX;
						}
					}
				} else {
					// different cell types, set to infinity
					distance = FLT_MAX;
				}
				// add to vector if distancethreshol holds
				if (distance < distancethreshold)
					distances[i].push_back({ c, distance });
			}
		}
	}
	return distances;
}

std::unordered_map<uint64_t /*actormashup*/, float /*distance*/> UtilityAlch::GetActorDistancesMap(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2, float distancethreshold, bool onlyiteriors)
{
	std::unordered_map<uint64_t /*actormashup*/, float /*distance*/> distances;
	float distance = 0.0f;
	RE::TESObjectCELL* cell = nullptr;
	RE::TESObjectCELL* cell2 = nullptr;
	// iterate over actors
#pragma omp parallel for private(cell, cell2, distance) num_threads(4) schedule(runtime)
	for (int i = 0; i < actors1.size(); i++) {
		// create empty new vector
		// iterate over all actors in the second list
		for (int c = 0; c < actors2.size(); c++) {
			// skip if actors are the same
			if (actors1[i]->GetFormID() == actors2[c]->GetFormID())
				continue;
			// calc distance

			// if actors in same worldspace -> calc distance directly
			// if actors in exterior / interior -> set to max value

			// if cells have same interior cell flag
			cell = actors1[i]->GetParentCell();
			cell2 = actors2[c]->GetParentCell();
			if (cell != nullptr && cell2 != nullptr) {
				if (!onlyiteriors && cell->IsInteriorCell() == cell2->IsInteriorCell()) {
					// if it actually is an interior cell, check whether its the same
					if (cell->IsInteriorCell()) {
						if (cell->GetFormID() == cell2->GetFormID()) {
							distance = sqrtf(actors1[i]->GetPosition().GetSquaredDistance(actors2[c]->GetPosition()));
							LOG3_5("{}calc, {}, {}, {}", Utility::PrintForm(actors1[i]), Utility::PrintForm(actors2[c]), distance);
						}
						// if its not the same set ifinity
						else {
							distance = FLT_MAX;
						}
					} else {
						// is exterior cell: check wordspace
						if (actors1[i]->GetWorldspaceID() == actors2[c]->GetWorldspaceID()) {
							// same worldspace
							distance = sqrtf(actors1[i]->GetPosition().GetSquaredDistance(actors2[c]->GetPosition()));
						} else {
							// different worlspace, set to infinity
							distance = FLT_MAX;
						}
					}
				} else if (onlyiteriors) {
					if (cell->IsInteriorCell()) {
						if (cell->GetFormID() == cell2->GetFormID()) {
							distance = sqrtf(actors1[i]->GetPosition().GetSquaredDistance(actors2[c]->GetPosition()));
						}
						// if its not the same set ifinity
						else {
							distance = FLT_MAX;
						}
					}
				} else {
					// different cell types, set to infinity
					distance = FLT_MAX;
				}
				//LOG3_1("{}[UtilityAlch] [GetActorDistancesMap] actor1: {},\tactor2: {},\tdistance: {}", actors1[i]->actor->GetName(), actors2[c]->actor->GetName(), distance);
				// add to vector if distancethreshol holds
				if (distance < distancethreshold) {
					// the entry is actorid1 concat actor1d2 -> actorid1 = 0x1, actorid2 = 0x30 -> 0x0000000100000030
					//distances.insert_or_assign((((uint64_t)actors1[i]->GetFormID()) << 32) | actors2[c]->GetFormID(), distance);
					#pragma omp critical (UtilityAlchGetActorDistancesMap)
					{
						distances.insert_or_assign((((uint64_t)i) << 32) | (uint64_t)c, distance);
					}
				}
			}
		}
	}
	return distances;
}

std::vector<std::pair<uint64_t /*actormashup*/, float /*distance*/>> UtilityAlch::GetActorDistancesList(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2, float distancethreshold, bool onlyiteriors)
{
	std::vector<std::pair<uint64_t /*actormashup*/, float /*distance*/>> distances;
	float distance = 0.0f;
	//RE::TESObjectCELL* cell = nullptr;
	RE::TESObjectCELL* cell2 = nullptr;
	ActorInfo::DynamicStats* ac1;
	ActorInfo::DynamicStats* ac2;

	distancethreshold *= distancethreshold;
	std::atomic_flag lock = ATOMIC_FLAG_INIT;
	// iterate over actors
#pragma omp parallel for private(cell, cell2, distance) num_threads(4) schedule(runtime)
	for (int i = 0; i < actors1.size(); i++) {
		// create empty new vector
		// iterate over all actors in the second list
		for (int c = 0; c < actors2.size(); c++) {
			// skip if actors are the same
			if (actors1[i]->GetFormID() == actors2[c]->GetFormID())
				continue;
			// calc distance

			ac1 = &actors1[i]->dynamic;
			ac2 = &actors2[c]->dynamic;

			// if actors in same worldspace -> calc distance directly
			// if actors in exterior / interior -> set to max value

			// if cells have same interior cell flag
			if (!onlyiteriors && ac1->_parentCellInterior == ac2->_parentCellInterior) {
				// if it actually is an interior cell, check whether its the same
				if (ac1->_parentCellInterior) {
					if (ac1->_parentCellID == cell2->GetFormID()) {
						distance = (ac1->_position.GetSquaredDistance(ac2->_position));
						//LOG3_5("{}calc, {}, {}, {}", Utility::PrintForm(actors1[i]), Utility::PrintForm(actors2[c]), distance);
					}
					// if its not the same set ifinity
					else {
						distance = FLT_MAX;
					}
				} else {
					// is exterior cell: check wordspace
					if (ac1->_parentWorldSpaceID == ac2->_parentWorldSpaceID) {
						// same worldspace
						distance = (ac1->_position.GetSquaredDistance(ac2->_position));
					} else {
						// different worlspace, set to infinity
						distance = FLT_MAX;
					}
				}
			} else if (onlyiteriors) {
				if (ac1->_parentCellInterior) {
					if (ac1->_parentCellID == cell2->GetFormID()) {
						distance = (ac1->_position.GetSquaredDistance(ac2->_position));
					}
					// if its not the same set ifinity
					else {
						distance = FLT_MAX;
					}
				}
			} else {
				// different cell types, set to infinity
				distance = FLT_MAX;
			}
			//LOG3_1("{}[UtilityAlch] [GetActorDistancesMap] actor1: {},\tactor2: {},\tdistance: {}", actors1[i]->actor->GetName(), actors2[c]->actor->GetName(), distance);
			// add to vector if distancethreshol holds
			if (distance < distancethreshold) {
				// the entry is actorid1 concat actor1d2 -> actorid1 = 0x1, actorid2 = 0x30 -> 0x0000000100000030
				//distances.insert_or_assign((((uint64_t)actors1[i]->GetFormID()) << 32) | actors2[c]->GetFormID(), distance);
				{
					Spinlock guard(lock);
					distances.push_back({ (((uint64_t)i) << 32) | (uint64_t)c, distance });
				}
			}
		}
	}
	return distances;
}

uint64_t UtilityAlch::Sum(std::vector<uint64_t> vec)
{
	uint64_t sum = 0;
	for (int i = 0; i < vec.size(); i++)
		sum += vec[i];
	return sum;
}

std::string UtilityAlch::Concat(std::vector<std::string> vec)
{
	std::string result = "|";
	for (auto& str : vec)
		result += str + "|";
	return result;
}

std::string UtilityAlch::Concat(std::set<std::string> vec)
{
	std::string result = "|";
	for (auto& str : vec)
		result += str + "|";
	return result;
}

std::string UtilityAlch::Concat(std::set<RE::FormID> vec)
{
	std::string result = "|";
	for (auto& id : vec)
		result += GetHex(id) + "|";
	return result;
}

std::string UtilityAlch::Concat(std::vector<std::tuple<Diseases::Disease, float /*chance*/, float /*scale*/>>* vec) {
	std::string result = "|";
	for (auto& [dis, chance, scale] : *vec)
		result += ToString(dis) + "|";
	return result;
}

