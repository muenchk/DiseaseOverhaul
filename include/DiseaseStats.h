#pragma once

#include "WeatherInfo.h"
#include "CellInfo.h"


class DiseaseStats
{
public:

	float LastGameTime;
	std::vector<DiseaseInfo*> diseases;

	DiseaseStats()
	{
		LastGameTime = RE::Calendar::GetSingleton()->GetDaysPassed();
	}

	/// <summary>
	/// finds a specific disease in the list of diseases of this actor
	/// </summary>
	/// <param name="value"></param>
	/// <returns></returns>
	DiseaseInfo* FindDisease(Diseases::Disease value);

	/// <summary>
	/// progress a specific disease [value] by [points] advancement points
	/// </summary>
	/// <param name="value"></param>
	/// <param name="actor"></param>
	/// <param name="points"></param>
	/// <returns>whether the actor should die</returns>
	[[nodiscard]] bool ProgressDisease(RE::Actor* actor, Diseases::Disease value, float points);

	/// <summary>
	/// Forces an increase in stage of a disease
	/// </summary>
	/// <param name="data"></param>
	/// <param name="actor"></param>
	/// <param name="value"></param>
	/// <returns>whether the actor should die</returns>
	bool ForceIncreaseStage(RE::Actor* actor, Diseases::Disease value);

	/// <summary>
	/// Forces a decrease in stage of a disease
	/// </summary>
	/// <param name="data"></param>
	/// <param name="actor"></param>
	/// <param name="value"></param>
	void ForceDecreaseStage(RE::Actor* actor, Diseases::Disease value);

	/// <summary>
	/// cleans up unused DiseaseInfo objects
	/// </summary>
	void CleanDiseases();

	/// <summary>
	/// Resets all diseaseinfo of the actor
	/// </summary>
	void Reset();
};
