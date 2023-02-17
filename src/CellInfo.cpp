#include "CellInfo.h"
#include "Data.h"

std::vector<Diseases::Disease> CellInfo::GetPossibleInfections()
{
	std::vector<Diseases::Disease> ret;
	auto data = Data::GetSingleton();
	if (IsIntenseCold())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kIntenseCold].begin(), data->spreadingDiseaseMap[Spreading::kIntenseCold].end());
	if (IsIntenseHeat())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kIntenseHeat].begin(), data->spreadingDiseaseMap[Spreading::kIntenseHeat].end());
	if (IsSwamp())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kInSwamp].begin(), data->spreadingDiseaseMap[Spreading::kInSwamp].end());
	if (IsAshland())
		ret.insert(ret.end(), data->spreadingDiseaseMap[Spreading::kInAshland].begin(), data->spreadingDiseaseMap[Spreading::kInAshland].end());
	return ret;
}
