#pragma once

#include "Disease.h"

class WeatherInfo
{
public:
	RE::TESWeather* weather;
	EnumType weathertype = WeatherTypes::kNone;

	bool IsAshstorm()
	{
		return weathertype & WeatherTypes::kAshstorm;
	}

	bool IsBlizzard()
	{
		return weathertype & WeatherTypes::kBlizzard;
	}
	bool IsCold()
	{
		return weathertype & WeatherTypes::kCold |
		       weathertype & WeatherTypes::kSnow |
		       weathertype & WeatherTypes::kBlizzard;
	}
	bool IsHeat()
	{
		return weathertype & WeatherTypes::kHeat |
		       weathertype & WeatherTypes::kHeatWave;
	}
	bool IsSnow()
	{
		return weathertype & WeatherTypes::kSnow;
	}
	bool IsRain()
	{
		return weathertype & WeatherTypes::kRain;
	}
	bool IsSandstorm()
	{
		return weathertype & WeatherTypes::kSandstorm;
	}
	bool IsStormy()
	{
		return weathertype & WeatherTypes::kStormy;
	}
	bool IsThunderstorm()
	{
		return weathertype & WeatherTypes::kThunderstorm;
	}
	bool IsWindy()
	{
		return weathertype & WeatherTypes::kWindy;
	}
	bool IsIntenseCold()
	{
		return weathertype & WeatherTypes::kBlizzard;
	}
	bool IsIntenseHeat()
	{
		return weathertype & WeatherTypes::kHeatWave;
	}

	std::vector<Diseases::Disease> GetPossibleInfections();
};
