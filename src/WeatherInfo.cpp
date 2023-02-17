#include "WeatherInfo.h"
#include "Data.h"

std::vector<Diseases::Disease> WeatherInfo::GetPossibleInfections()
{
	std::vector<Diseases::Disease> ret;
	auto data = Data::GetSingleton();
	if (IsIntenseCold())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kIntenseCold].begin(), data->spreadingDiseaseMap[Spreading::kIntenseCold].end());
	if (IsIntenseHeat())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kIntenseHeat].begin(), data->spreadingDiseaseMap[Spreading::kIntenseHeat].end());
	if (IsAshstorm())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kInAshstorm].begin(), data->spreadingDiseaseMap[Spreading::kInAshstorm].end());
	if (IsBlizzard())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kInBlizzard].begin(), data->spreadingDiseaseMap[Spreading::kInBlizzard].end());
	if (IsSandstorm())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kInSandstorm].begin(), data->spreadingDiseaseMap[Spreading::kInSandstorm].end());
	if (IsRain())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kInRain].begin(), data->spreadingDiseaseMap[Spreading::kInRain].end());
	if (IsCold())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kIsCold].begin(), data->spreadingDiseaseMap[Spreading::kIsCold].end());
	if (IsHeat())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kIsHeat].begin(), data->spreadingDiseaseMap[Spreading::kIsHeat].end());
	return ret;
}
