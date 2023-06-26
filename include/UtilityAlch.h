#pragma once 

#include <climits>
#include "Utility.h"

class UtilityAlch : public Utility
{
public:
	static std::string ToString(Diseases::Disease value);

	static std::string ToString(Infectivity value)
	{
		switch (value) {
		case Infectivity::kNone:
			return "None";
		case Infectivity::kLow:
			return "Low";
		case Infectivity::kNormal:
			return "Normal";
		case Infectivity::kHigher:
			return "Higher";
		case Infectivity::kHigh:
			return "High";
		case Infectivity::kVeryHigh:
			return "Very High";
		}
		return "Unknown";
	}

	static std::string ToString(DiseaseStatus value)
	{
		switch (value) {
		case DiseaseStatus::kProgressing:
			return "Progressing";
		case DiseaseStatus::kRegressing:
			return "Regressing";
		case DiseaseStatus::kInfection:
			return "Infection";
		}
		return "Unknown";
	}

	/// <summary>
	/// Returns a string representing the [disease]
	/// </summary>
	/// <param name="disease"></param>
	/// <returns></returns>
	static std::string ToString(std::shared_ptr<Disease>& disease);

	/// <summary>
	/// Returns a string representing the [stage]
	/// </summary>
	/// <param name="stage"></param>
	/// <returns></returns>
	static std::string ToString(std::shared_ptr<DiseaseStage>& stage);

	/// <summary>
	/// Calculates a chance
	/// </summary>
	/// <param name="chance"></param>
	/// <returns></returns>
	static bool CalcChance(float chance);

	/// <summary>
	/// Returns only infected actors from a list of actors
	/// </summary>
	/// <param name="actors"></param>
	/// <returns></returns>
	static std::vector<std::shared_ptr<ActorInfo>> GetInfectedActors(std::vector<std::shared_ptr<ActorInfo>> actors)
	{
		std::vector<std::shared_ptr<ActorInfo>> infected;
		for (int i = 0; i < actors.size(); i++) {
			for (int c = 0; c < actors[i]->dinfo.diseases.size(); c++)
				// counts as infected as long as the disease broke out (including incubation)
				if (actors[i]->dinfo.diseases[c]->status != DiseaseStatus::kInfection)
					infected.push_back(actors[i]);
		}
		return infected;
	}

	/// <summary>
	/// Returns only infected actors that are progressing from a list of actors
	/// </summary>
	/// <param name="actors"></param>
	/// <returns></returns>
	static std::vector<std::shared_ptr<ActorInfo>> GetProgressingActors(std::vector<std::shared_ptr<ActorInfo>> actors)
	{
		std::vector<std::shared_ptr<ActorInfo>> infected;
		for (int i = 0; i < actors.size(); i++) {
			for (int c = 0; c < actors[i]->dinfo.diseases.size(); c++)
				// counts as infected as long as the disease broke out (including incubation)
				if (actors[i]->dinfo.diseases[c]->status == DiseaseStatus::kProgressing)
					infected.push_back(actors[i]);
		}
		return infected;
	}

	/// <summary>
	/// Returns only actors infected with [disease] and progressing from a list of actors
	/// </summary>
	/// <param name="actors"></param>
	/// <returns></returns>
	static std::vector<std::shared_ptr<ActorInfo>> GetProgressingActors(std::vector<std::shared_ptr<ActorInfo>> actors, Diseases::Disease disease)
	{
		std::vector<std::shared_ptr<ActorInfo>> infected;
		for (int i = 0; i < actors.size(); i++) {
			for (int c = 0; c < actors[i]->dinfo.diseases.size(); c++)
				// counts as infected as long as the disease broke out (including incubation)
				if (actors[i]->dinfo.diseases[c]->status == DiseaseStatus::kProgressing && actors[i]->dinfo.diseases[c]->disease == disease)
					infected.push_back(actors[i]);
		}
		return infected;
	}

	/// <summary>
	/// Calculates the distance of all actors in the vector
	/// </summary>
	/// <param name="actors"></param>
	/// <returns></returns>
	static std::vector<std::tuple<std::shared_ptr<ActorInfo>, std::shared_ptr<ActorInfo>, float /*distance*/>> GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors)
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
	
	/// <summary>
	/// Calculates the distance of between all actors of [actors1] and [actors2]
	/// </summary>
	/// <param name="actors1"></param>
	/// <param name="actors2"></param>
	/// <returns>vector with size sizeof([actors1]) containing vectors of size sizeof([actors2])</returns>
	static std::vector<std::vector<std::tuple<int /*idx in actors2*/, float /*distance*/>>> GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2)
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

	/// <summary>
	/// Calculates the distance of between all actors of [actors1] and [actors2] and returns only those with a maximum distance of [distancethreshold]
	/// </summary>
	/// <param name="actors1"></param>
	/// <param name="actors2"></param>
	/// <param name="distancethreshold">threshold for distances returned</param>
	/// <returns>vector with size sizeof([actors1]) containing vectors of size sizeof([actors2])</returns>
	static std::vector<std::vector<std::tuple<int /*idx in actors2*/, float /*distance*/>>> GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2, float distancethreshold)
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

	/// <summary>
	/// Calculates the distance of between all actors of [actors1] and [actors2] and returns a map with the distances lesser [distancethreshold]
	/// </summary>
	/// <param name="actors1"></param>
	/// <param name="actors2"></param>
	/// <param name="distancethreshold">threshold for distances returned</param>
	/// <param name="onlyinteriors">only calculates distances for interior cells</param>
	/// <returns>vector with size sizeof([actors1]) containing vectors of size sizeof([actors2])</returns>
	static std::unordered_map<uint64_t /*actormashup*/, float /*distance*/> GetActorDistancesMap(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2, float distancethreshold, bool onlyiteriors = false)
	{
		std::unordered_map<uint64_t /*actormashup*/, float /*distance*/> distances;
		float distance = 0.0f;
		RE::TESObjectCELL* cell = nullptr;
		RE::TESObjectCELL* cell2 = nullptr;
		// iterate over actors
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
								loginfo("calc, {}, {}, {}", Utility::PrintForm(actors1[i]), Utility::PrintForm(actors2[c]), distance);
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
						distances.insert_or_assign((((uint64_t)i) << 32) | (uint64_t)c, distance);
					}
				}
			}
		}
		return distances;
	}

	static uint64_t Sum(std::vector<uint64_t> vec)
	{
		uint64_t sum = 0;
		for (int i = 0; i < vec.size(); i++)
			sum += vec[i];
		return sum;
	}
};
