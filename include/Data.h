#pragma once

#include <unordered_map>
#include <unordered_set>

#include "ActorInfo.h"

class Data
{
private:
	/// <summary>
	/// map that contains information about any npc that has entered combat during runtime
	/// </summary>
	std::unordered_map<uint32_t, ActorInfo*> actorinfoMap;
	/// <summary>
	/// array that contains all diseases
	/// </summary>
	Disease* diseases[Diseases::kMaxValue];
	/// <summary>
	/// dummy disease that helps avoid nullptr dereferences
	/// </summary>
	Disease dummyDisease;

	/// <summary>
	/// map that contains information about game cells loaded during runtime
	/// </summary>
	std::unordered_map<uint32_t, CellInfo*> cellinfoMap;
	/// <summary>
	/// map that contains information about game weathers loaded during runtime
	/// </summary>
	std::unordered_map<uint32_t, WeatherInfo*> weatherinfoMap;

public:
	/// <summary>
	/// map that contains cells and disease spread modifiers for that cell
	/// </summary>
	std::unordered_map<uint32_t, CellDiseaseProperties*> cellMap;

	/// <summary>
	/// map that contains information about weathers
	/// </summary>
	std::unordered_map<uint32_t, WeatherDiseaseProperties*> weatherMap;

	/// <summary>
	/// map that contains information associated with testures
	/// </summary>
	std::unordered_map<uint32_t, TextureDiseaseProperties*> textureMap;

	/// <summary>
	/// map that contains diseases an actor/etc. may be infected with at initialization
	/// </summary>
	std::unordered_map<uint32_t, std::vector<Diseases::Disease>*> diseasesAssoc;

	std::vector<Diseases::Disease> spreadingDiseaseMap[Spreading::kMaxValue];

	/// <summary>
	/// returns a pointer to a static Data object
	/// </summary>
	/// <returns></returns>
	static Data* GetSingleton();
	/// <summary>
	/// Returns an actorinfo object with information about [actor]
	/// </summary>
	/// <param name="actor">the actor to find</param>
	/// <returns></returns>
	ActorInfo* FindActor(RE::Actor* actor);
	/// <summary>
	/// Returns a cellinfo object with information about [cell]
	/// </summary>
	/// <param name="cell"></param>
	/// <returns></returns>
	CellInfo* FindCell(RE::TESObjectCELL* cell);
	/// <summary>
	/// Returns a weatherinfo object with information about [weather]
	/// </summary>
	/// <param name="weather"></param>
	/// <returns></returns>
	WeatherInfo* FindWeather(RE::TESWeather* weather);
	/// <summary>
	/// Returns the map with all available actorinformation
	/// </summary>
	/// <returns></returns>
	std::unordered_map<uint32_t, ActorInfo*>* ActorInfoMap();
	/// <summary>
	/// Resets information about actors
	/// </summary>
	void ResetActorInfoMap();

	Disease* GetDisease(Diseases::Disease);

	std::vector<ActorInfo*> AirInfections(RE::TESObjectCELL* cell);
	std::vector<ActorInfo*> ParticleInfections(RE::TESObjectCELL* cell);
	bool IsIntenseCold(RE::TESObjectCELL* cell, RE::TESWeather* weather);
	bool IsIntenseHeat(RE::TESObjectCELL* cell, RE::TESWeather* weather);
	bool IsDessert(RE::TESObjectCELL* cell);
	bool IsIceland(RE::TESObjectCELL* cell);
	bool IsAshland(RE::TESObjectCELL* cell);
	bool IsSwamp(RE::TESObjectCELL* cell);
	bool IsAshstorm(RE::TESWeather* weather);
	bool IsSandstorm(RE::TESWeather* weather);
	bool IsBlizzard(RE::TESWeather* weather);

	void InitDiseases();

}; 
