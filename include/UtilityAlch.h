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
	static std::vector<std::shared_ptr<ActorInfo>> GetInfectedActors(std::vector<std::shared_ptr<ActorInfo>> actors);

	/// <summary>
	/// Returns only infected actors that are progressing from a list of actors
	/// </summary>
	/// <param name="actors"></param>
	/// <returns></returns>
	static std::vector<std::shared_ptr<ActorInfo>> GetProgressingActors(std::vector<std::shared_ptr<ActorInfo>> actors);

	/// <summary>
	/// Returns only actors infected with [disease] and progressing from a list of actors
	/// </summary>
	/// <param name="actors"></param>
	/// <returns></returns>
	static std::vector<std::shared_ptr<ActorInfo>> GetProgressingActors(std::vector<std::shared_ptr<ActorInfo>> actors, Diseases::Disease disease);

	/// <summary>
	/// Calculates the distance of all actors in the vector
	/// </summary>
	/// <param name="actors"></param>
	/// <returns></returns>
	static std::vector<std::tuple<std::shared_ptr<ActorInfo>, std::shared_ptr<ActorInfo>, float /*distance*/>> GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors);
	
	/// <summary>
	/// Calculates the distance of between all actors of [actors1] and [actors2]
	/// </summary>
	/// <param name="actors1"></param>
	/// <param name="actors2"></param>
	/// <returns>vector with size sizeof([actors1]) containing vectors of size sizeof([actors2])</returns>
	static std::vector<std::vector<std::tuple<int /*idx in actors2*/, float /*distance*/>>> GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2);

	/// <summary>
	/// Calculates the distance of between all actors of [actors1] and [actors2] and returns only those with a maximum distance of [distancethreshold]
	/// </summary>
	/// <param name="actors1"></param>
	/// <param name="actors2"></param>
	/// <param name="distancethreshold">threshold for distances returned</param>
	/// <returns>vector with size sizeof([actors1]) containing vectors of size sizeof([actors2])</returns>
	static std::vector<std::vector<std::tuple<int /*idx in actors2*/, float /*distance*/>>> GetActorDistances(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2, float distancethreshold);

	/// <summary>
	/// Calculates the distance of between all actors of [actors1] and [actors2] and returns a map with the distances lesser [distancethreshold]
	/// </summary>
	/// <param name="actors1"></param>
	/// <param name="actors2"></param>
	/// <param name="distancethreshold">threshold for distances returned</param>
	/// <param name="onlyinteriors">only calculates distances for interior cells</param>
	/// <returns>vector with size sizeof([actors1]) containing vectors of size sizeof([actors2])</returns>
	static std::unordered_map<uint64_t /*actormashup*/, float /*distance*/> GetActorDistancesMap(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2, float distancethreshold, bool onlyiteriors = false);

	static std::vector<std::pair<uint64_t /*actormashup*/, float /*distance*/>> GetActorDistancesList(std::vector<std::shared_ptr<ActorInfo>> actors1, std::vector<std::shared_ptr<ActorInfo>> actors2, float distancethreshold, bool onlyiteriors = false);

	static uint64_t Sum(std::vector<uint64_t> vec);

	static std::string Concat(std::vector<std::string> vec);
	static std::string Concat(std::set<std::string> vec);
	static std::string Concat(std::set<RE::FormID> vec);
	static std::string Concat(std::vector<std::tuple<Diseases::Disease, float /*chance*/, float /*scale*/>>* vec);
};
