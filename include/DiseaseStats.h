#pragma once

#include "WeatherInfo.h"
#include "CellInfo.h"

class ActorInfo;

class DiseaseStats
{
public:

	float LastGameTime;
	std::vector<DiseaseInfo*> diseases;
	EnumType disflags = 0;
	EnumType disflagsprog = 0;

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
	[[nodiscard]] bool ProgressDisease(std::shared_ptr<ActorInfo> acinfo, Diseases::Disease value, float points);

	/// <summary>
	/// Forces an increase in stage of a disease
	/// </summary>
	/// <param name="data"></param>
	/// <param name="actor"></param>
	/// <param name="value"></param>
	/// <returns>whether the actor should die</returns>
	bool ForceIncreaseStage(std::shared_ptr<ActorInfo> acinfo, Diseases::Disease value);

	/// <summary>
	/// Forces a decrease in stage of a disease
	/// </summary>
	/// <param name="data"></param>
	/// <param name="actor"></param>
	/// <param name="value"></param>
	void ForceDecreaseStage(std::shared_ptr<ActorInfo> acinfo, Diseases::Disease value);

	/// <summary>
	/// returns whether there is an active disease
	/// </summary>
	/// <returns></returns>
	bool IsInfected();
	bool IsInfected(Diseases::Disease dis);
	bool IsInfectedProgressing(Diseases::Disease dis);

	/// <summary>
	/// cleans up unused DiseaseInfo objects
	/// </summary>
	void CleanDiseases();

	/// <summary>
	/// Calcs Flags for infections
	/// </summary>
	void CalcFlags();

	/// <summary>
	/// Resets all diseaseinfo of the actor
	/// </summary>
	void Reset();
};
