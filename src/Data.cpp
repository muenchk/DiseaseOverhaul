#include "Data.h"
#include "Logging.h"
#include "UtilityAlch.h"
#include "Season.h"




Data* Data::GetSingleton()
{
	static Data singleton;
	return std::addressof(singleton);
}

ActorInfo* Data::FindActor(RE::Actor* actor)
{
	ActorInfo* acinfo = nullptr;
	auto itr = actorinfoMap.find(actor->GetFormID());
	if (itr == actorinfoMap.end()) {
		acinfo = new ActorInfo(actor, 0, 0, 0, 0, 0);
		actorinfoMap.insert_or_assign(actor->GetFormID(), acinfo);
	} else if (itr->second == nullptr || itr->second->actor == nullptr || itr->second->actor->GetFormID() == 0 || itr->second->actor->GetFormID() != actor->GetFormID()) {
		// either delete acinfo, deleted actor, actor fid 0 or acinfo belongs to wrong actor
		if (itr->second)
			delete itr->second;
		actorinfoMap.erase(actor->GetFormID());
		acinfo = new ActorInfo(actor, 0, 0, 0, 0, 0);
		actorinfoMap.insert_or_assign(actor->GetFormID(), acinfo);
	} else {
		acinfo = itr->second;
		if (acinfo->citems == nullptr)
			acinfo->citems = new ActorInfo::CustomItems();
	}
	return acinfo;
}

CellInfo* Data::FindCell(RE::TESObjectCELL* cell)
{
	// use this if we get an invalid object, and need to return something nonetheless. since its not in the map
	// no one will ever try to delete it
	static CellInfo statcinfo;
	if (cell == nullptr)
		return &statcinfo;
	CellInfo* cinfo = nullptr;
	auto itr = cellinfoMap.find(cell->GetFormID());
	if (itr == cellinfoMap.end()) {
		cinfo = new CellInfo();
		// calc and assign flags
	} else if (itr->second == nullptr || itr->second->cell == nullptr || itr->second->seasoncalc != Season::GetCurrentSeason()) {
		if (itr->second)
			delete itr->second;
		cellinfoMap.erase(cell->GetFormID());
		cinfo = new CellInfo();
		// calc and assign flags
	} else {
		cinfo = itr->second;
		return cinfo;
	}
	// calc and assign flags
	// the other pathes have already exited the function by now
	cinfo->cell = cell;
	Season::Season season = Season::GetCurrentSeason();
	cinfo->seasoncalc = season;
	auto iter = cellMap.find(cell->GetFormID());
	if (iter != cellMap.end()) {
		if (iter->second) {
			if (iter->second->type & CellTypes::kAshland)
				cinfo->celltype |= CellInfoType::kAshland;
			else if (iter->second->type & CellTypes::kCold)
				cinfo->celltype |= CellInfoType::kCold;
			else if (iter->second->type & CellTypes::kDessert) {
				cinfo->celltype |= CellInfoType::kDessert;
				cinfo->celltype |= CellInfoType::kIntenseHeat;
			} else if (iter->second->type & CellTypes::kHeat)
				cinfo->celltype |= CellInfoType::kHeat;
			else if (iter->second->type & CellTypes::kIceland) {
				cinfo->celltype |= CellInfoType::kIceland;
				cinfo->celltype |= CellInfoType::kIntenseCold;
			} else if (iter->second->type & CellTypes::kSnow) {
				cinfo->celltype |= CellInfoType::kCold;
				cinfo->celltype |= CellInfoType::kSnow;
			} else if (iter->second->type & CellTypes::kSwamp)
				cinfo->celltype |= CellInfoType::kSwamp;
			else if (iter->second->type & CellTypes::kSummerCold && season == Season::Season::kSummer)
				cinfo->celltype |= CellInfoType::kCold;
			else if (iter->second->type & CellTypes::kSummerHot && season == Season::Season::kSummer)
				cinfo->celltype |= CellInfoType::kHeat;
			else if (iter->second->type & CellTypes::kWinterCold && season == Season::Season::kWinter)
				cinfo->celltype |= CellInfoType::kCold;
			else if (iter->second->type & CellTypes::kWinterHot && season == Season::Season::kWinter)
				cinfo->celltype |= CellInfoType::kHeat;
		}
	}
	RE::FormID formid = 0;
	if (cell->cellLand &&
		cell->cellLand->loadedData) {
		for (int i = 0; i < 4; i++) {
			if (cell->cellLand->loadedData->defQuadTextures[i] &&
				cell->cellLand->loadedData->defQuadTextures[i]->textureSet) {
				formid = cell->cellLand->loadedData->defQuadTextures[i]->textureSet->GetFormID();
				auto itra = textureMap.find(formid);
				if (itra != textureMap.end()) {
					if (itra->second) {
						if (itra->second->type & TextureTypes::kIceland) {
							cinfo->celltype |= CellInfoType::kIceland;
							cinfo->celltype |= CellInfoType::kIntenseCold;
						} else if (itra->second->type & TextureTypes::kAshland) {
							cinfo->celltype |= CellInfoType::kAshland;
						} else if (itra->second->type & TextureTypes::kCold) {
							cinfo->celltype |= CellInfoType::kCold;
						} else if (itra->second->type & TextureTypes::kDessert) {
							cinfo->celltype |= CellInfoType::kDessert;
							cinfo->celltype |= CellInfoType::kIntenseHeat;
						} else if (itra->second->type & TextureTypes::kHeat) {
							cinfo->celltype |= CellInfoType::kHeat;
						} else if (itra->second->type & TextureTypes::kSnow) {
							cinfo->celltype |= CellInfoType::kCold;
							cinfo->celltype |= CellInfoType::kSnow;
						} else if (itra->second->type & TextureTypes::kSwamp) {
							cinfo->celltype |= CellInfoType::kSwamp;
						}

					}
				}
			}
		}
	}
	cellinfoMap.insert_or_assign(cell->GetFormID(), cinfo);

	return cinfo;
}

WeatherInfo* Data::FindWeather(RE::TESWeather* weather)
{
	// use this if we get an invalid object, and need to return something nonetheless. since its not in the map
	// no one will ever try to delete it
	static WeatherInfo statwinfo;
	if (weather == nullptr)
		return &statwinfo;
	WeatherInfo* winfo = nullptr;
	auto itr = weatherinfoMap.find(weather->GetFormID());
	if (itr == weatherinfoMap.end()) {
		winfo = new WeatherInfo();
		// calc and assign flags
	} else if (itr->second == nullptr || itr->second->weather == nullptr) {
		if (itr->second)
			delete itr->second;
		weatherinfoMap.erase(weather->GetFormID());
		winfo = new WeatherInfo();
		// calc and assign flags
	} else {
		winfo = itr->second;
		return winfo;
	}
	// calc and assign flags
	// all other pathes have already returned
	winfo->weather = weather;
	auto iter = weatherMap.find(weather->GetFormID());
	if (iter != weatherMap.end()) {
		if (iter->second) {

		}
	}
	// use game weather flags
	auto itra = weather->sounds.begin();
	while (itra != weather->sounds.end()) {
		if ((*itra)->type & RE::TESWeather::SoundType::kThunder) {
			winfo->weathertype |= WeatherTypes::kThunderstorm;
		}
		static_cast<void>(itra++);
	}
	if (weather->data.flags & RE::TESWeather::WeatherDataFlag::kRainy)
		winfo->weathertype |= WeatherTypes::kRain;
	if (weather->data.flags & RE::TESWeather::WeatherDataFlag::kSnow)
		winfo->weathertype |= WeatherTypes::kSnow;

	return winfo;
}

std::unordered_map<uint32_t, ActorInfo*>* Data::ActorInfoMap()
{
	return &actorinfoMap;
}

void Data::ResetActorInfoMap()
{
	auto itr = actorinfoMap.begin();
	while (itr != actorinfoMap.end()) {
		if (itr->second)
			itr->second->_boss = false;
		itr->second->citems->Reset();
		itr++;
	}
}

bool Data::IsIntenseCold(RE::TESObjectCELL* cell, RE::TESWeather* weather)
{
	CellInfo* cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kIntenseCold)
		return true;
	if (weather) {
		auto itr = weatherMap.find(weather->GetFormID());
		if (itr != weatherMap.end()) {
			if (itr->second && 
				(itr->second->type & WeatherTypes::kBlizzard || 
				 itr->second->type & WeatherTypes::kCold))
				return true;
		}
	}
	return false;
}

bool Data::IsIntenseHeat(RE::TESObjectCELL* cell, RE::TESWeather* weather)
{
	CellInfo* cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kIntenseHeat)
		return true;
	if (weather) {
		auto itr = weatherMap.find(weather->GetFormID());
		if (itr != weatherMap.end()) {
			if (itr->second &&
				(itr->second->type & WeatherTypes::kHeat))
				return true;
		}
	}
	return false;
}

bool Data::IsDessert(RE::TESObjectCELL* cell)
{
	CellInfo* cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kDessert)
		return true;
	return false;
}

bool Data::IsIceland(RE::TESObjectCELL* cell)
{
	CellInfo* cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kIceland)
		return true;
	return false;
}

bool Data::IsAshland(RE::TESObjectCELL* cell)
{
	CellInfo* cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kAshland)
		return true;
	return false;
}

bool Data::IsSwamp(RE::TESObjectCELL* cell)
{
	CellInfo* cinfo = FindCell(cell);
	if (cinfo->celltype & CellInfoType::kSwamp)
		return true;
	return false;
}

bool Data::IsAshstorm(RE::TESWeather* weather)
{
	if (!weather)
		return false;
	auto itr = weatherMap.find(weather->GetFormID());
	if (itr != weatherMap.end()) {
		if (itr->second && itr->second->type & WeatherTypes::kAshstorm)
			return true;
	} else
		return false; // there is no information so assume false
	return false;
}

bool Data::IsSandstorm(RE::TESWeather* weather)
{
	if (!weather)
		return false;
	auto itr = weatherMap.find(weather->GetFormID());
	if (itr != weatherMap.end()) {
		if (itr->second && itr->second->type & WeatherTypes::kSandstorm)
			return true;
	} else
		return false;  // there is no information so assume false
	return false;
}

bool Data::IsBlizzard(RE::TESWeather* weather)
{
	if (!weather)
		return false;
	auto itr = weatherMap.find(weather->GetFormID());
	if (itr != weatherMap.end()) {
		if (itr->second && itr->second->type & WeatherTypes::kBlizzard)
			return true;
	} else
		return false;  // there is no information so assume false
	return false;
}

Disease* Data::GetDisease(Diseases::Disease value)
{
	if (diseases[value] == nullptr) {
		return &dummyDisease;
	}
	return diseases[value];
}

void Data::InitDiseases()
{
	auto datahandler = RE::TESDataHandler::GetSingleton();
	DiseaseStage* stage = nullptr;
	std::pair<int, float> tint; 
	RE::TESForm* tmp = nullptr;
	RE::SpellItem* spell = nullptr;
	for (int i = 0; i < Diseases::kMaxValue; i++) {
		diseases[i] = new Disease();
		diseases[i]->_stageInfection = new DiseaseStage(); 
	}
	// Ash Woe Blight
	{
		diseases[Diseases::kAshWoeBlight]->_name = UtilityAlch::ToString(Diseases::kAshWoeBlight);
		diseases[Diseases::kAshWoeBlight]->_numstages = 4;
		diseases[Diseases::kAshWoeBlight]->_stageInfection->_advancementThreshold = 100;
		diseases[Diseases::kAshWoeBlight]->_baseProgressionPoints = 0;
		diseases[Diseases::kAshWoeBlight]->_baseInfectionReductionPoints = 1;
		diseases[Diseases::kAshWoeBlight]->_baseInfectionChance = 3;
		diseases[Diseases::kAshWoeBlight]->_validModifiers = PermanentModifiers::kPotionOfCureBlight | PermanentModifiers::kSpellOfCureBlight | PermanentModifiers::kShrine;
		diseases[Diseases::kAshWoeBlight]->_stages = new DiseaseStage*[diseases[Diseases::kAshWoeBlight]->_numstages + 1];
		// infection
		{
			stage = new DiseaseStage();
			stage->_advancementThreshold = 100;
			stage->_advancementTime = 0;
			stage->_infectivity = Infectivity::kNone;
			diseases[Diseases::kAshWoeBlight]->_stageInfection = stage;
		}
		// incubation
		{
			stage = new DiseaseStage();
			stage->_advancementThreshold = 2160;
			stage->_advancementTime = 3.0;
			stage->_infectivity = Infectivity::kLow;
			// ashstorm
			tint = { 100 /*guarantied chance*/, 6.0f /*points per tick*/ };
			stage->_spreading[Spreading::kInAshstorm] = tint;
			// ashland
			tint = { 100, 1.0f };
			stage->_spreading[Spreading::kInAshland] = tint;
			tint = { 100, 1.0f };
			stage->_spreading[Spreading::kParticle] = tint;
			stage->effect = nullptr;
			diseases[Diseases::kAshWoeBlight]->_stages[0] = stage;
		}
		// stage 1
		{
			stage = new DiseaseStage();
			stage->_advancementThreshold = 2160;
			stage->_advancementTime = 3.0;
			stage->_infectivity = Infectivity::kNormal;
			// ashstorm
			tint = { 100 /*guarantied chance*/, 8.0f /*points per tick*/ };
			stage->_spreading[Spreading::kInAshstorm] = tint;
			// ashland
			tint = { 100, 1.0f };
			stage->_spreading[Spreading::kInAshland] = tint;
			tint = { 100, 1.0f };
			stage->_spreading[Spreading::kParticle] = tint;
			if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x5907, Settings::PluginName, "")) != nullptr) {
				if ((spell = tmp->As<RE::SpellItem>()) != nullptr)
					stage->effect = spell;
			}
			diseases[Diseases::kAshWoeBlight]->_stages[1] = stage;
		}
		// stage 2
		{
			stage = new DiseaseStage();
			stage->_advancementThreshold = 3600;
			stage->_advancementTime = 5.0;
			stage->_infectivity = Infectivity::kHigher;
			// ashstorm
			tint = { 100 /*guarantied chance*/, 10.0f /*points per tick*/ };
			stage->_spreading[Spreading::kInAshstorm] = tint;
			// ashland
			tint = { 100, 1.5f };
			stage->_spreading[Spreading::kInAshland] = tint;
			tint = { 100, 1.0f };
			stage->_spreading[Spreading::kParticle] = tint;
			// AEXTDiseaseAshWoeBlight2
			if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x5908, Settings::PluginName, "")) != nullptr) {
				if ((spell = tmp->As<RE::SpellItem>()) != nullptr)
					stage->effect = spell;
			}
			diseases[Diseases::kAshWoeBlight]->_stages[2] = stage;
		}
		// stage 3
		{
			stage = new DiseaseStage();
			stage->_advancementThreshold = 3600;
			stage->_advancementTime = 5.0;
			stage->_infectivity = Infectivity::kHigher;
			// ashstorm
			tint = { 100 /*guarantied chance*/, 12.0f /*points per tick*/ };
			stage->_spreading[Spreading::kInAshstorm] = tint;
			// ashland
			tint = { 100, 1.5f };
			stage->_spreading[Spreading::kInAshland] = tint;
			tint = { 100, 1.0f };
			stage->_spreading[Spreading::kParticle] = tint;
			// AEXTDiseaseAshWoeBlight3
			if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x5909, Settings::PluginName, "")) != nullptr) {
				if ((spell = tmp->As<RE::SpellItem>()) != nullptr)
					stage->effect = spell;
			}
			diseases[Diseases::kAshWoeBlight]->_stages[3] = stage;
		}
		// stage 4
		{
			stage = new DiseaseStage();
			stage->_advancementThreshold = 5040;
			stage->_advancementTime = 7.0;
			stage->_infectivity = Infectivity::kHigher;
			// ashstorm
			tint = { 100 /*guarantied chance*/, 15.0f /*points per tick*/ };
			stage->_spreading[Spreading::kInAshstorm] = tint;
			// ashland
			tint = { 100, 2.0f };
			stage->_spreading[Spreading::kInAshland] = tint;
			tint = { 100, 1.0f };
			stage->_spreading[Spreading::kParticle] = tint;
			// AEXTDiseaseAshWoeBlight3
			if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x590A, Settings::PluginName, "")) != nullptr) {
				if ((spell = tmp->As<RE::SpellItem>()) != nullptr)
					stage->effect = spell;
			}
			diseases[Diseases::kAshWoeBlight]->_stages[4] = stage;
		}
		diseases[Diseases::kAshWoeBlight]->CalcFlags();
		spreadingDiseaseMap[Spreading::kInAshland].push_back(Diseases::kAshChancre);
		spreadingDiseaseMap[Spreading::kInAshstorm].push_back(Diseases::kAshChancre);
	}
}
