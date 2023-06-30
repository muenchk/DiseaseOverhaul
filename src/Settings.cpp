#include <fstream>
#include <iostream>
#include <type_traits>
#include <utility>
#include <string_view>
#include <chrono>
#include <set>
#include <time.h>
#include <random>
#include <tuple>
#include <vector>

#include "Compatibility.h"
#include "Settings.h"
#include "Data.h"
#include "UtilityAlch.h"

using Comp = Compatibility;

using ActorStrength = ActorStrength;
using ItemStrength = ItemStrength;
using ItemType = Settings::ItemType;

static std::mt19937 randi((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
/// <summary>
/// trims random numbers to 1 to RR
/// </summary>
static std::uniform_int_distribution<signed> randRR(1, RandomRange);
static std::uniform_int_distribution<signed> rand100(1, 100);

#pragma region Interfaces

void Settings::Interfaces::RequestAPIs()
{
	loginfo("[SETTINGS] [RequestInterfaces]");
	// get tmp api
	if (!tdm_api) {
		loginfo("[SETTINGS] [RequestInterfaces] Trying to get True Directional Movement API");
		tdm_api = reinterpret_cast<TDM_API::IVTDM1*>(TDM_API::RequestPluginAPI(TDM_API::InterfaceVersion::V1));
		if (tdm_api) {
			loginfo("[SETTINGS] [RequestInterfaces] Acquired True Directional Movement API");
		} else {
			loginfo("[SETTINGS] [RequestInterfaces] Failed to get True Directional Movement API");
		}
	}
	// get nupinter
	if (!nupinter) {
		loginfo("[SETTINGS] [RequestInterfaces] Trying to get NPCsUsePotions API");
		nupinter = reinterpret_cast<NPCsUsePotions::NUPInterface*>(NPCsUsePotions::RequestPluginAPI());
		if (nupinter) {
			loginfo("[SETTINGS] [RequestInterfaces] Acquired NPCsUsePotions API");
		} else {
			loginfo("[SETTINGS] [RequestInterfaces] Failed to get NPCsUsePotions API");
		}
	}
}

#pragma endregion

#pragma region Settings

void Settings::LoadGameObjects()
{
	LOG_1("{}[Settings] [LoadGameObjects]");
	// load our stuff like necessary forms
	// get VendorItemPotion keyword, if we don't find this, potion detection will be nearly impossible
	Settings::GameObj::VendorItemPotion = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0008CDEC);
	if (Settings::GameObj::VendorItemPotion == nullptr) {
		logger::info("[INIT] Couldn't find VendorItemPotion Keyword in game.");
	}
	Settings::GameObj::VendorItemPoison = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0008CDED);
	if (Settings::GameObj::VendorItemPoison == nullptr) {
		logger::info("[INIT] Couldn't find VendorItemPoison Keyword in game.");
	}
	Settings::GameObj::VendorItemFood = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0008CDEA);
	if (Settings::GameObj::VendorItemFood == nullptr) {
		logger::info("[INIT] Couldn't find VendorItemFood Keyword in game.");
	}
	Settings::GameObj::VendorItemFoodRaw = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000A0E56);
	if (Settings::GameObj::VendorItemFoodRaw == nullptr) {
		logger::info("[INIT] Couldn't find VendorItemFoodRaw Keyword in game.");
	}
	Settings::GameObj::ActorTypeDwarven = RE::TESForm::LookupByID<RE::BGSKeyword>(0x1397A);
	if (Settings::GameObj::ActorTypeDwarven == nullptr) {
		logger::info("[INIT] Couldn't find ActorTypeDwarven Keyword in game.");
	}
	Settings::GameObj::ActorTypeCreature = RE::TESForm::LookupByID<RE::BGSKeyword>(0x13795);
	if (Settings::GameObj::ActorTypeCreature == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find ActorTypeCreature Keyword in game.");
	}
	Settings::GameObj::ActorTypeAnimal = RE::TESForm::LookupByID<RE::BGSKeyword>(0x13798);
	if (Settings::GameObj::ActorTypeAnimal == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find ActorTypeAnimal Keyword in game.");
	}
	Settings::GameObj::Vampire = RE::TESForm::LookupByID<RE::BGSKeyword>(0xA82BB);
	if (Settings::GameObj::ActorTypeDwarven == nullptr) {
		logger::info("[INIT] Couldn't find Vampire Keyword in game.");
	}

	Settings::GameObj::CurrentFollowerFaction = RE::TESForm::LookupByID<RE::TESFaction>(0x0005C84E);
	if (Settings::GameObj::CurrentFollowerFaction == nullptr) {
		logger::info("[INIT] Couldn't find CurrentFollowerFaction Faction in game.");
	}
	Settings::GameObj::CurrentHirelingFaction = RE::TESForm::LookupByID<RE::TESFaction>(0xbd738);
	if (Settings::GameObj::CurrentHirelingFaction == nullptr) {
		logger::info("[INIT] Couldn't find CurrentHirelingFaction Faction in game.");
	}
	Settings::GameObj::WerewolfFaction = RE::TESForm::LookupByID<RE::TESFaction>(0x43594);
	if (Settings::GameObj::CurrentHirelingFaction == nullptr) {
		logger::info("[INIT] Couldn't find WerewolfFaction Faction in game.");
	}
	Settings::GameObj::CreatureFaction = RE::TESForm::LookupByID<RE::TESFaction>(0x13);
	if (Settings::GameObj::CurrentHirelingFaction == nullptr) {
		logger::info("[INIT] Couldn't find CfreatureFaction Faction in game.");
	}

	Settings::GameObj::WerewolfImmunity = RE::TESForm::LookupByID<RE::SpellItem>(0xF5BA0);
	if (Settings::GameObj::ActorTypeDwarven == nullptr) {
		logger::info("[INIT] Couldn't find WerewolfImmunity Spell in game.");
	}

	loginfo("[SETTINGS] [InitGameStuff] Load Game Stuff");
	RE::TESDataHandler* datahandler = RE::TESDataHandler::GetSingleton();
	const RE::TESFile* file = nullptr;
	uint32_t index = 0;
	for (int i = 0; i <= 254; i++) {
		file = datahandler->LookupLoadedModByIndex((uint8_t)i);
		if (file) {
			pluginnames[i] = std::string(file->GetFilename());
			index = (uint32_t)i << 24;
			pluginNameMap.insert_or_assign(pluginnames[i], index);
			pluginIndexMap.insert_or_assign(index, pluginnames[i]);
		} else
			pluginnames[i] = "";
	}
	// 0xFF... is reserved for objects created by the game during runtime
	pluginnames[255] = "runtime";
	for (int i = 0; i <= 4095; i++) {
		file = datahandler->LookupLoadedLightModByIndex((uint16_t)i);
		if (file) {
			pluginnames[256 + i] = std::string(file->GetFilename());
			index = 0xFE000000 | ((uint32_t)i << 12);
			pluginNameMap.insert_or_assign(pluginnames[256 + i], index);
			pluginIndexMap.insert_or_assign(index, pluginnames[256 + i]);
		} else
			pluginnames[256 + i] = "";
	}

	Settings::GameObj::Equip_LeftHand = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F43);
	Settings::GameObj::Equip_RightHand = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F42);
	Settings::GameObj::Equip_EitherHand = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F44);
	Settings::GameObj::Equip_BothHands = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F45);
	Settings::GameObj::Equip_Shield = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x141E8);
	Settings::GameObj::Equip_Voice = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x25BEE);
	Settings::GameObj::Equip_Potion = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x35698);


	// get ITMPoisonUse as sound for applying poisons
	// ITMPoisonUse
	Settings::GameObj::PoisonUse = RE::TESForm::LookupByID<RE::BGSSoundDescriptorForm>(0x106614);
	// get ITMPotionUse for sound fixes
	// ITMPotionUse
	Settings::GameObj::PotionUse = RE::TESForm::LookupByID<RE::BGSSoundDescriptorForm>(0xB6435);
	// ITMFoodEat
	Settings::GameObj::FoodEat = RE::TESForm::LookupByID<RE::BGSSoundDescriptorForm>(0xCAF94);

	/// load disease objects
}

std::set<RE::FormID> Settings::CalcRacesWithoutPotionSlot()
{
	LOG_1("{}[Settings] [ExcludeRacesWithoutPotionSlot]");
	std::set<RE::FormID> ret;
	RE::BSTArray<RE::TESRace*> races = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESRace>();
	LOG1_1("{}[Settings] [ExcludeRacesWithoutPotionSlot] found {} races.", races.size());
	for (RE::TESRace* race : races) {
		bool potionenabled = false;
		for (auto slot : race->equipSlots) {
			if (slot->GetFormID() == 0x35698)
				potionenabled = true;
		}
		if (potionenabled == false) {
			ret.insert(race->GetFormID());
			LOG1_1("{}[Settings] [ExcludeRacesWithoutPotionSlot] {} does not have potion slot and has been excluded.", Utility::PrintForm(race));
		}
	}
	LOG_1("{}[Settings] [ExcludeRacesWithoutPotionSlot] end");
	return ret;
}

void Settings::LoadDistrConfigNUP()
{
	// set to false, to avoid other funcions running stuff on our variables
	Distribution::initialised = false;

	// disable generic logging, if load logging is disabled
	if (Logging::EnableLoadLog == false)
		Logging::EnableGenericLogging = false;

	// reset custom items
	Distribution::ResetCustomItems();
	// reset loaded rules
	Distribution::ResetRules();

	std::vector<std::string> files;
	auto constexpr folder = R"(Data\SKSE\Plugins\)";
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini") {
			if (auto path = entry.path().string(); path.rfind("NUP_DIST") != std::string::npos) {
				files.push_back(path);
			}
		}
	}
	if (files.empty()) {
	}
	// init datahandler
	auto datahandler = RE::TESDataHandler::GetSingleton();

	// change order of files handled, so that files that include "default" are loaded first, so other rules may override them
	int defind = 0;
	for (int k = 0; k < files.size(); k++) {
		if (Utility::ToLower(files[k]).find("default") != std::string::npos) {
			std::string tmp = files[defind];
			files[defind] = files[k];
			files[k] = tmp;
			defind++;
		}
	}
	for (int k = 0; k < files.size(); k++) {
		//loginfo("[SETTINGS] [LoadDistrRules] found Distribution configuration file: {}", files[k]);
	}

	// vector of splits, filename and line
	std::vector<std::tuple<std::vector<std::string>*, std::string, std::string>> attachments;
	std::vector<std::tuple<std::vector<std::string>*, std::string, std::string>> copyrules;

	const int chancearraysize = 5;


	// extract the rules from all files
	for (std::string file : files) {
		try {
			std::ifstream infile(file);
			if (infile.is_open()) {
				std::string line;
				while (std::getline(infile, line)) {
					std::string tmp = line;
					// we read another line
					// check if its empty or with a comment
					if (line.empty())
						continue;
					// remove leading spaces and tabs
					while (line.length() > 0 && (line[0] == ' ' || line[0] == '\t')) {
						line = line.substr(1, line.length() - 1);
					}
					// check again
					if (line.length() == 0 || line[0] == ';')
						continue;
					// now begin the actual processing
					std::vector<std::string>* splits = new std::vector<std::string>();
					// split the string into parts
					size_t pos = line.find('|');
					while (pos != std::string::npos) {
						splits->push_back(line.substr(0, pos));
						line.erase(0, pos + 1);
						pos = line.find("|");
					}
					if (line.length() != 0)
						splits->push_back(line);
					int splitindex = 0;
					// check wether we actually have a rule
					if (splits->size() < 3) {  // why 3? Cause first two fields are RuleVersion and RuleType and we don't accept empty rules.
						//logwarn("[Settings] [LoadDistrRules] Not a rule. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what rule version we have
					int ruleVersion = -1;
					try {
						ruleVersion = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what kind of rule we have
					int ruleType = -1;
					try {
						ruleType = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// now we can actually make differences for the different rule version and types
					switch (ruleVersion) {
					case 1:
						{
							switch (ruleType) {
							case 1:  // distribution rule
								{
									if (splits->size() != 25) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 25. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										delete splits;
										continue;
									}
									// next entry is the rulename, so we just set it
									Distribution::Rule* rule = new Distribution::Rule();
									rule->ruleVersion = ruleVersion;
									rule->ruleType = ruleType;
									rule->ruleName = splits->at(splitindex);
									////LOGLE1_2("[Settings] [LoadDistrRules] loading rule: {}", rule->ruleName);
									splitindex++;
									// now come the rule priority
									rule->rulePriority = -1;
									try {
										rule->rulePriority = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"RulePrio\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RulePrio\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes maxPotions
									rule->maxPotions = -1;
									try {
										rule->maxPotions = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"MaxPotions\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"MaxPotions\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion1Chance
									rule->potion1Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion1Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Potion1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion2Chance
									rule->potion2Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion2Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Potion2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion3Chance
									rule->potion3Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion3Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Potion3Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PotionAddChance
									rule->potionAdditionalChance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potionAdditionalChance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"PotionAddChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PotionsTierAdjust
									rule->potionTierAdjust = -1;
									try {
										rule->potionTierAdjust = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"PotionsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"PotionsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Fortify1Chance
									rule->fortify1Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortify1Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Fortify1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Fortify2Chance
									rule->fortify2Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortify2Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Fortify2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes maxPoisons
									rule->maxPoisons = -1;
									try {
										rule->maxPoisons = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"MaxPoisons\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"MaxPoisons\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison1Chance
									rule->poison1Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison1Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Poison1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison2Chance
									rule->poison2Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison2Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Poison2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison3Chance
									rule->poison3Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison3Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Poison3Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PoisonAddChance
									rule->poisonAdditionalChance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poisonAdditionalChance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"PoisonAddChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PoisonsTierAdjust
									rule->poisonTierAdjust = -1;
									try {
										rule->poisonTierAdjust = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"PoisonsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"PoisonsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}

									// now comes FoodChance
									rule->foodChance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->foodChance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"FoodChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}

									// get strings for the properties
									rule->assocObjects = splits->at(splitindex);
									splitindex++;
									rule->potionProperties = splits->at(splitindex);
									splitindex++;
									rule->fortifyproperties = splits->at(splitindex);
									splitindex++;
									rule->poisonProperties = splits->at(splitindex);
									splitindex++;
									rule->foodProperties = splits->at(splitindex);
									splitindex++;

									if (splits->at(splitindex) == "1")
										rule->allowMixed = true;
									else
										rule->allowMixed = false;
									splitindex++;

									bool error = false;

									// parse the associated objects
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> objects = Utility::ParseAssocObjects(rule->assocObjects, error, file, tmp, total);

									// parse the item properties
									std::vector<std::tuple<AlchemicEffect, float>> potioneffects = Utility::ParseAlchemyEffects(rule->potionProperties, error);
									rule->potionDistr = Utility::GetDistribution(potioneffects, RandomRange);
									rule->potionDistrChance = Utility::GetDistribution(potioneffects, RandomRange, true);
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} potion effects", rule->ruleName, rule->potionDistr.size());
									rule->validPotions = Utility::SumAlchemyEffects(rule->potionDistr, true);
									std::vector<std::tuple<AlchemicEffect, float>> poisoneffects = Utility::ParseAlchemyEffects(rule->poisonProperties, error);
									rule->poisonDistr = Utility::GetDistribution(poisoneffects, RandomRange);
									rule->poisonDistrChance = Utility::GetDistribution(poisoneffects, RandomRange, true);
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} poison effects", rule->ruleName, rule->poisonDistr.size());
									rule->validPoisons = Utility::SumAlchemyEffects(rule->poisonDistr, true);
									std::vector<std::tuple<AlchemicEffect, float>> fortifyeffects = Utility::ParseAlchemyEffects(rule->fortifyproperties, error);
									rule->fortifyDistr = Utility::GetDistribution(fortifyeffects, RandomRange);
									rule->fortifyDistrChance = Utility::GetDistribution(fortifyeffects, RandomRange, true);
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} fortify potion effects", rule->ruleName, rule->fortifyDistr.size());
									rule->validFortifyPotions = Utility::SumAlchemyEffects(rule->fortifyDistr, true);
									std::vector<std::tuple<AlchemicEffect, float>> foodeffects = Utility::ParseAlchemyEffects(rule->foodProperties, error);
									rule->foodDistr = Utility::GetDistribution(foodeffects, RandomRange);
									rule->foodDistrChance = Utility::GetDistribution(foodeffects, RandomRange, true);
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} food effects", rule->ruleName, rule->foodDistr.size());
									rule->validFood = Utility::SumAlchemyEffects(rule->foodDistr, true);

									std::pair<int, Distribution::Rule*> tmptuple = { rule->rulePriority, rule };

									// assign rules to search parameters
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} associated objects", rule->ruleName, objects.size());
									for (int i = 0; i < objects.size(); i++) {
										switch (std::get<0>(objects[i])) {
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kCombatStyle:
										case Distribution::AssocType::kClass:
										case Distribution::AssocType::kRace:
										case Distribution::AssocType::kKeyword:
											if (auto item = Distribution::_assocMap.find(std::get<1>(objects[i])); item != Distribution::_assocMap.end()) {
												if (std::get<1>(item->second)->rulePriority < rule->rulePriority)
													Distribution::_assocMap.insert_or_assign(std::get<1>(objects[i]), tmptuple);
											} else {
												Distribution::_assocMap.insert_or_assign(std::get<1>(objects[i]), tmptuple);
											}
											break;
										case Distribution::AssocType::kNPC:
										case Distribution::AssocType::kActor:
											if (auto item = Distribution::_npcMap.find(std::get<1>(objects[i])); item != Distribution::_npcMap.end()) {
												if (item->second->rulePriority < rule->rulePriority)
													Distribution::_npcMap.insert_or_assign(std::get<1>(objects[i]), rule);
											} else {
												Distribution::_npcMap.insert_or_assign(std::get<1>(objects[i]), rule);
											}
											break;
										}
									}
									// add rule to the list of rules and we are finished! probably.
									Distribution::_rules.push_back(rule);
									if (rule->ruleName == DefaultRuleName && (Distribution::defaultRule == nullptr ||
																				 rule->rulePriority > Distribution::defaultRule->rulePriority))
										Distribution::defaultRule = rule;
									delete splits;
									//LOGLE1_2("[Settings] [LoadDistrRules] rule {} successfully loaded.", rule->ruleName);
								}
								break;
							case 2:  // distribution attachement
								{
									attachments.push_back({ splits, file, tmp });
								}
								// dont delete splits since we need it later
								break;
							case 3:  // declare boss
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										if (std::get<0>(items[i]) & Distribution::AssocType::kActor ||
											std::get<0>(items[i]) & Distribution::AssocType::kNPC ||
											std::get<0>(items[i]) & Distribution::AssocType::kFaction ||
											std::get<0>(items[i]) & Distribution::AssocType::kKeyword ||
											std::get<0>(items[i]) & Distribution::AssocType::kRace) {
											Distribution::_bosses.insert(std::get<1>(items[i]));
											//LOGLE1_2("[Settings] [LoadDistrRules] declared {} as boss.", Utility::GetHex(std::get<1>(items[i])));
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 4:  // exclude object
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kActor:
										case Distribution::AssocType::kNPC:
											Distribution::_excludedNPCs.insert(std::get<1>(items[i]));
											break;
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kKeyword:
										case Distribution::AssocType::kRace:
											Distribution::_excludedAssoc.insert(std::get<1>(items[i]));
											break;
										case Distribution::AssocType::kItem:
											Distribution::_excludedItems.insert(std::get<1>(items[i]));
											break;
										}
										if (Logging::EnableLoadLog) {
											if (std::get<0>(items[i]) & Distribution::AssocType::kActor ||
												std::get<0>(items[i]) & Distribution::AssocType::kNPC) {
												//LOGLE1_2("[Settings] [LoadDistrRules] excluded actor {} from distribution.", Utility::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) & Distribution::AssocType::kFaction) {
												//LOGLE1_2("[Settings] [LoadDistrRules] excluded faction {} from distribution.", Utility::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) & Distribution::AssocType::kKeyword) {
												//LOGLE1_2("[Settings] [LoadDistrRules] excluded keyword {} from distribution.", Utility::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) & Distribution::AssocType::kItem) {
												//LOGLE1_2("[Settings] [LoadDistrRules] excluded item {} from distribution.", Utility::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) & Distribution::AssocType::kRace) {
												//LOGLE1_2("[Settings] [LoadDistrRules] excluded race {} from distribution.", Utility::GetHex(std::get<1>(items[i])));
											}
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 5:  // exclude baseline
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kKeyword:
										case Distribution::AssocType::kRace:
											Distribution::_baselineExclusions.insert(std::get<1>(items[i]));
											break;
										}

										if (Logging::EnableLoadLog) {
											if (std::get<0>(items[i]) & Distribution::AssocType::kFaction) {
												//LOGLE1_2("[Settings] [LoadDistrRules] excluded faction {} from base line distribution.", Utility::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) & Distribution::AssocType::kKeyword) {
												//LOGLE1_2("[Settings] [LoadDistrRules] excluded keyword {} from base line distribution.", Utility::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) & Distribution::AssocType::kRace) {
												//LOGLE1_2("[Settings] [LoadDistrRules] excluded race {} from base line distribution.", Utility::GetHex(std::get<1>(items[i])));
											}
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 6:  // copy rule
								{
									copyrules.push_back({ splits, file, tmp });
								}
								break;
							case 7:  // whitelist rule
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kItem:
											Distribution::_whitelistItems.insert(std::get<1>(items[i]));
											break;
										case Distribution::AssocType::kNPC:
										case Distribution::AssocType::kActor:
										case Distribution::AssocType::kClass:
										case Distribution::AssocType::kCombatStyle:
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kKeyword:
										case Distribution::AssocType::kRace:
											Distribution::_whitelistNPCs.insert(std::get<1>(items[i]));
											if (Logging::EnableLoadLog) {
												//LOGLE1_2("[Settings] [LoadDistrRules] whitelisted object {}.", Utility::GetHex(std::get<1>(items[i])));
											}
											break;
										default:
											//LOGLE1_2("[Settings] [LoadDistrRules] cannot whitelist object {}.", Utility::GetHex(std::get<1>(items[i])));
											break;
										}
										if (Logging::EnableLoadLog) {
											if (std::get<0>(items[i]) & Distribution::AssocType::kItem) {
												//LOGLE1_2("[Settings] [LoadDistrRules] whitelisted item {}.", Utility::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) & Distribution::AssocType::kRace) {
											}
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 8:  // custom object distribution
								{
									if (splits->size() != 5) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 5. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string id = splits->at(splitindex);
									splitindex++;
									Distribution::CustomItemStorage* citems = new Distribution::CustomItemStorage();
									citems->id = id;
									// parse associated obj
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									bool errorcustom = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> assocobj = Utility::ParseAssocObjects(assoc, error, file, tmp, total);

									// parse items associated
									assoc = splits->at(splitindex);
									splitindex++;
									error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID, int32_t, CustomItemFlag, int8_t, bool, std::vector<std::tuple<uint64_t, uint32_t, std::string>>, std::vector<std::tuple<uint64_t, uint32_t, std::string>>, bool>> associtm = Utility::ParseCustomObjects(assoc, errorcustom, file, tmp);
									RE::TESForm* tmpf = nullptr;
									RE::TESBoundObject* tmpb = nullptr;
									RE::AlchemyItem* alch = nullptr;
									// parse items first: if there are no items we don't need to do anything further
									for (int i = 0; i < associtm.size(); i++) {
										tmpf = nullptr;
										tmpb = nullptr;
										switch (std::get<0>(associtm[i])) {
										case Distribution::AssocType::kItem:
											{
												tmpf = RE::TESForm::LookupByID(std::get<1>(associtm[i]));
												if (tmpf) {
													tmpb = tmpf->As<RE::TESBoundObject>();
													alch = tmpf->As<RE::AlchemyItem>();
												}
												//LOGL1_3("{}[Settings] [LoadDstrRules] Flag converted: {}", static_cast<uint64_t>(std::get<3>(associtm[i])));
												if (tmpb) {
													if (std::get<5>(associtm[i]))
														Distribution::_excludedItems.insert(std::get<2>(associtm[i]));
													switch (std::get<3>(associtm[i])) {
													case CustomItemFlag::Object:
														{
															//LOGL_3("{}[Settings] [LoadDstrRules] Path 1");
															CustomItem* cit = new CustomItem();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = tmpb;
															citems->items.push_back(cit);
														}
														break;
													case CustomItemFlag::DeathObject:
														{
															//LOGL_3("{}[Settings] [LoadDstrRules] Path 2");

															CustomItem* cit = new CustomItem();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = tmpb;
															citems->death.push_back(cit);
														}
														break;
													case CustomItemFlag::Food:
														//LOGL_3("{}[Settings] [LoadDstrRules] Path 3");
														if (alch) {
															CustomItemAlch* cit = new CustomItemAlch();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = alch;
															citems->food.push_back(cit);
														}
														break;
													case CustomItemFlag::Fortify:
														//LOGL_3("{}[Settings] [LoadDstrRules] Path 4");
														if (alch) {
															CustomItemAlch* cit = new CustomItemAlch();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = alch;
															citems->fortify.push_back(cit);
														}
														break;
													case CustomItemFlag::Poison:
														//LOGL_3("{}[Settings] [LoadDstrRules] Path 5");
														if (alch) {
															CustomItemAlch* cit = new CustomItemAlch();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = alch;
															citems->poisons.push_back(cit);
														}
														break;
													case CustomItemFlag::Potion:
														//LOGL_3("{}[Settings] [LoadDstrRules] Path 6");
														if (alch) {
															CustomItemAlch* cit = new CustomItemAlch();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = alch;
															citems->potions.push_back(cit);
														}
														break;
													}
												} else {
													//LOGLE1_2("[Settings] [LoadDistrRules] custom rule for item {} cannot be applied, due to the item not being an TESBoundObject.", Utility::GetHex(std::get<1>(associtm[i])));
												}
											}
											break;
										}
									}
									if (citems->items.size() == 0 &&
										citems->death.size() == 0 &&
										citems->food.size() == 0 &&
										citems->fortify.size() == 0 &&
										citems->poisons.size() == 0 &&
										citems->potions.size() == 0) {
										//logwarn("[Settings] [LoadDistrRules] rule does not contain any items. file: {}, rule:\"{}\"", file, tmp);
										delete citems;
										continue;
									}

									int cx = 0;
									// now parse associations
									for (int i = 0; i < assocobj.size(); i++) {
										switch (std::get<0>(assocobj[i])) {
										case Distribution::AssocType::kActor:
										case Distribution::AssocType::kNPC:
										case Distribution::AssocType::kClass:
										case Distribution::AssocType::kCombatStyle:
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kKeyword:
										case Distribution::AssocType::kRace:
											citems->assocobjects.insert(std::get<1>(assocobj[i]));
											auto iter = Distribution::_customItems.find(std::get<1>(assocobj[i]));
											if (iter != Distribution::_customItems.end()) {
												std::vector<Distribution::CustomItemStorage*> vec = iter->second;
												vec.push_back(citems);
												Distribution::_customItems.insert_or_assign(std::get<1>(assocobj[i]), vec);
												cx++;
											} else {
												std::vector<Distribution::CustomItemStorage*> vec = { citems };
												Distribution::_customItems.insert_or_assign(std::get<1>(assocobj[i]), vec);
												cx++;
											}
										}
										if (Logging::EnableLoadLog) {
											if (std::get<0>(assocobj[i]) & Distribution::AssocType::kKeyword) {
											} else if (std::get<0>(assocobj[i]) & Distribution::AssocType::kRace) {
											} else if (std::get<0>(assocobj[i]) & Distribution::AssocType::kFaction) {
											} else if (std::get<0>(assocobj[i]) & Distribution::AssocType::kCombatStyle) {
											} else if (std::get<0>(assocobj[i]) & Distribution::AssocType::kClass) {
											} else if (std::get<0>(assocobj[i]) & Distribution::AssocType::kActor ||
													   std::get<0>(assocobj[i]) & Distribution::AssocType::kNPC) {
											}
										}
										//LOGL_2("{}[Settings] [LoadDistrRules] attached custom rule to specific objects");
									}
									if (cx == 0 && total == 0) {
										auto iter = Distribution::_customItems.find(0x0);
										if (iter != Distribution::_customItems.end()) {
											std::vector<Distribution::CustomItemStorage*> vec = iter->second;
											vec.push_back(citems);
											Distribution::_customItems.insert_or_assign(0x0, vec);
											cx++;
										} else {
											std::vector<Distribution::CustomItemStorage*> vec = { citems };
											Distribution::_customItems.insert_or_assign(0x0, vec);
											cx++;
										}
										//LOGL_2("{}[Settings] [LoadDistrRules] attached custom rule to everything");
									}

									// since we are done delete splits
									delete splits;
								}
								break;
							case 9:  // exclude plugin
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string plugin = splits->at(splitindex);
									splitindex++;
									uint32_t index = Utility::Mods::GetPluginIndex(plugin);

									if (index != 0x1) {
										// index is a normal mod
										Distribution::_excludedPlugins.insert(index);
										//LOGLE2_2("[Settings] [LoadDistrRules] Rule 9 excluded plugin {} with index {}", plugin, Utility::GetHex(index));
									} else {
										//LOGLE1_2("[Settings] [LoadDistrRules] Rule 9 cannot exclude plugin {}. It is either not loaded or not present", plugin);
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 10:  // set item strength
								{
									if (splits->size() != 4) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 4. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string strength = splits->at(splitindex);
									splitindex++;
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									ItemStrength str = ItemStrength::kWeak;
									// arse item strength
									try {
										str = static_cast<ItemStrength>(std::stoi(strength));
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"ItemStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"ItemStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::exception&) {
										//logwarn("[Settings] [LoadDistrRules] generic expection in field \"ItemStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kItem:
											Distribution::_itemStrengthMap.insert_or_assign(std::get<1>(items[i]), str);
											break;
										}
										if (Logging::EnableLoadLog) {
											if (std::get<0>(items[i]) & Distribution::AssocType::kItem) {
												//LOGLE1_2("[Settings] [LoadDistrRules] set item strength {}.", Utility::GetHex(std::get<1>(items[i])));
											} else {
												//logwarn("[Settings] [LoadDistrRules] rule 10 is not applicable to object {}.", Utility::GetHex(std::get<1>(items[i])));
											}
										}
									}

									// since we are done delete splits
									delete splits;
								}
								break;
							case 11:  // adjust actor strength
								{
									if (splits->size() != 4) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 4. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string strength = splits->at(splitindex);
									splitindex++;
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									int str = 0;
									// arse item strength
									try {
										str = std::stoi(strength);
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"RelativeActorStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RelativeActorStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::exception&) {
										//logwarn("[Settings] [LoadDistrRules] generic expection in field \"RelativeActorStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}
									if (str != 0) {
										for (int i = 0; i < items.size(); i++) {
											switch (std::get<0>(items[i])) {
											case Distribution::AssocType::kActor:
											case Distribution::AssocType::kNPC:
											case Distribution::AssocType::kFaction:
											case Distribution::AssocType::kKeyword:
											case Distribution::AssocType::kRace:
												Distribution::_actorStrengthMap.insert_or_assign(std::get<1>(items[i]), str);
												break;
											}
											if (Logging::EnableLoadLog) {
												if (std::get<0>(items[i]) & Distribution::AssocType::kActor ||
													std::get<0>(items[i]) & Distribution::AssocType::kNPC ||
													std::get<0>(items[i]) & Distribution::AssocType::kFaction ||
													std::get<0>(items[i]) & Distribution::AssocType::kKeyword ||
													std::get<0>(items[i]) & Distribution::AssocType::kRace) {
													//LOGLE1_2("[Settings] [LoadDistrRules] set relative actor strength {}.", Utility::GetHex(std::get<1>(items[i])));
												} else {
													//logwarn("[Settings] [LoadDistrRules] rule 11 is not applicable to object {}.", Utility::GetHex(std::get<1>(items[i])));
												}
											}
										}
									}

									// since we are done delete splits
									delete splits;
								}
								break;
							case 12:  // whitelist plugin
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string pluginname = splits->at(splitindex);
									splitindex++;
									bool error = false;
									auto forms = Utility::Mods::GetFormsInPlugin<RE::AlchemyItem>(pluginname);
									for (int i = 0; i < forms.size(); i++) {
										Distribution::_whitelistItems.insert(forms[i]->GetFormID());
										//if (Logging::EnableLoadLog)
											//LOGLE3_2("[Settings] [LoadDistrRules] whitelisted item. id: {}, name: {}, plugin: {}.", Utility::GetHex(forms[i]->GetFormID()), forms[i]->GetName(), pluginname);
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 13:  // follower detection
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kFaction:
											Distribution::_followerFactions.insert(std::get<1>(items[i]));
											break;
										}
									}
									delete splits;
								}
								break;
							case 14:  // poison dosage item based
								{
									// version, type, enforce, assocobjects (items), dosage or setting
									if (splits->size() != 5) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 5. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string senforce = splits->at(splitindex);
									splitindex++;
									bool enforce = false;
									try {
										if (std::stol(senforce) == 1)
											enforce = true;
									} catch (std::exception&) {
										enforce = false;
									}

									std::string assoc = splits->at(splitindex);
									splitindex++;
									std::string sdosage = splits->at(splitindex);
									splitindex++;
									bool setting = false;
									int dosage = 0;
									if (sdosage == "setting") {
										setting = true;
										dosage = 1;
									} else {
										try {
											dosage = std::stoi(sdosage);
										} catch (std::exception&) {
											//logwarn("[Settings] [LoadDistrRules] expection in field \"Dosage\". file: {}, rule:\"{}\"", file, tmp);
											delete splits;
											continue;
										}
									}
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										if (std::get<0>(items[i]) == Distribution::AssocType::kItem) {
											Distribution::_dosageItemMap.insert_or_assign(std::get<1>(items[i]), std::tuple<bool, bool, int>{ enforce, setting, dosage });
										}
									}
									delete splits;
								}
								break;
							case 15:  // poison dosage effect based
								{
									// version, type, enforce, effect, dosage or setting
									if (splits->size() != 5) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 5. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string senforce = splits->at(splitindex);
									splitindex++;
									bool enforce = false;
									try {
										if (std::stol(senforce) == 1)
											enforce = true;
									} catch (std::exception&) {
										enforce = false;
									}

									std::string seffects = splits->at(splitindex);
									splitindex++;
									std::string sdosage = splits->at(splitindex);
									splitindex++;
									bool setting = false;
									int dosage = 0;
									if (sdosage == "setting") {
										setting = true;
										dosage = 1;
									} else {
										try {
											dosage = std::stoi(sdosage);
										} catch (std::exception&) {
											//logwarn("[Settings] [LoadDistrRules] expection in field \"Dosage\". file: {}, rule:\"{}\"", file, tmp);
											delete splits;
											continue;
										}
									}
									bool error = false;
									std::vector<std::tuple<AlchemicEffect, float>> effects = Utility::ParseAlchemyEffects(seffects, error);
									for (int i = 0; i < effects.size(); i++) {
										AlchemicEffect effect = std::get<0>(effects[i]);
										if (effect != AlchemicEffect::kNone) {
											Distribution::_dosageEffectMap.insert_or_assign(effect, std::tuple<bool, bool, int>{ enforce, setting, dosage });
										}
									}
									delete splits;
								}
								break;
							case 16:  // exclude effect
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string effect = splits->at(splitindex);
									splitindex++;
									AlchemicEffect e = 0;
									try {
										e = effect;
									} catch (std::exception&) {
										//logwarn("[Settings] [LoadDistrRules] expection in field \"Effect\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}
									if (e != AlchemicEffect::kNone) {
										Distribution::_excludedEffects.insert(e);
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 17:  // exclude plugin NPCs
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string plugin = splits->at(splitindex);
									splitindex++;
									uint32_t plugindex = Utility::Mods::GetPluginIndex(plugin);
									if (plugindex != 0x1) {
										// valid plugin index
										Distribution::_excludedPlugins_NPCs.insert(plugindex);
										//LOGLE1_2("[Settings] [LoadDistrRules] Rule 17 excluded plugin {}. It is either not loaded or not present", plugin);
									} else {
										//LOGLE1_2("[Settings] [LoadDistrRules] Rule 17 cannot exclude plugin {}. It is either not loaded or not present", plugin);
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 18:  // whitelist plugin NPCs
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string plugin = splits->at(splitindex);
									splitindex++;
									uint32_t plugindex = Utility::Mods::GetPluginIndex(plugin);
									if (plugindex != 0x1) {
										// valid plugin index
										Distribution::_whitelistNPCsPlugin.insert(plugindex);
										//LOGLE1_2("[Settings] [LoadDistrRules] Rule 18 whitelisted plugin {}. It is either not loaded or not present", plugin);
									} else {
										//LOGLE1_2("[Settings] [LoadDistrRules] Rule 18 cannot whitelist plugin {}. It is either not loaded or not present", plugin);
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 19:  // mark item as alcoholic
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kItem:
											Distribution::_alcohol.insert(std::get<1>(items[i]));
											//LOGLE1_2("[Settings] [LoadDistrRules] marked {} as alcoholic", Utility::GetHex(std::get<1>(items[i])));
											break;
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 20:  // define alchemy effect
								{
									if (splits->size() != 4) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									std::string effect = splits->at(splitindex);
									splitindex++;
									AlchemicEffect e = 0;
									try {
										e = effect;
									} catch (std::exception&) {
										//logwarn("[Settings] [LoadDistrRules] expection in field \"Effect\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}
									if (e != AlchemicEffect::kNone) {
										bool error = false;
										int total = 0;
										auto items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
										for (int i = 0; i < items.size(); i++) {
											switch (std::get<0>(items[i])) {
											case Distribution::AssocType::kEffectSetting:
												Distribution::_magicEffectAlchMap.insert_or_assign(std::get<1>(items[i]), e);
												//LOGLE2_2("[Settings] [LoadDistrRules] fixed {} to effect {}", Utility::GetHex(std::get<1>(items[i])), Utility::ToString(e));
												break;
											}
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 21:  // disallow player usage of items
								{
									if (splits->size() != 3) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kEffectSetting:
										case Distribution::AssocType::kItem:
											Distribution::_excludedItemsPlayer.insert(std::get<1>(items[i]));
											//LOGLE1_2("[Settings] [LoadDistrRules] excluded {} for the player", Utility::GetHex(std::get<1>(items[i])));
											break;
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							default:
								//logwarn("[Settings] [LoadDistrRules] Rule type does not exist. file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								break;
							}
						}
						break;
					case 2:
						{
							switch (ruleType) {
							case 1:  // distribution rule
								{
									if (splits->size() != 33) {
										//logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 33. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										delete splits;
										continue;
									}
									// next entry is the rulename, so we just set it
									Distribution::Rule* rule = new Distribution::Rule();
									rule->ruleVersion = ruleVersion;
									rule->ruleType = ruleType;
									rule->ruleName = splits->at(splitindex);
									//LOGLE1_2("[Settings] [LoadDistrRules] loading rule: {}", rule->ruleName);
									splitindex++;
									// now come the rule priority
									rule->rulePriority = -1;
									try {
										rule->rulePriority = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"RulePrio\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RulePrio\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes maxPotions
									rule->maxPotions = -1;
									try {
										rule->maxPotions = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"MaxPotions\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"MaxPotions\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion1Chance
									rule->potion1Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion1Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Potion1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion2Chance
									rule->potion2Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion2Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Potion2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion3Chance
									rule->potion3Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion3Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Potion3Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion4Chance
									rule->potion4Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion4Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Potion4Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PotionAddChance
									rule->potionAdditionalChance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potionAdditionalChance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"PotionAddChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PotionsTierAdjust
									rule->potionTierAdjust = -1;
									try {
										rule->potionTierAdjust = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"PotionsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"PotionsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes maxFortify
									rule->maxFortify = -1;
									try {
										rule->maxFortify = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"MaxFortify\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"MaxFortify\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Fortify1Chance
									rule->fortify1Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortify1Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Fortify1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Fortify2Chance
									rule->fortify2Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortify2Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Fortify2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Fortify3Chance
									rule->fortify3Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortify3Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Fortify3Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Fortify4Chance
									rule->fortify4Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortify4Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Fortify4Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes FortifyAddChance
									rule->fortifyAdditionalChance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortifyAdditionalChance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"FortifyAddChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes FortifyTierAdjust
									rule->fortifyTierAdjust = -1;
									try {
										rule->fortifyTierAdjust = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"FortifyTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"FortifyTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes maxPoisons
									rule->maxPoisons = -1;
									try {
										rule->maxPoisons = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"MaxPoisons\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"MaxPoisons\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison1Chance
									rule->poison1Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison1Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Poison1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison2Chance
									rule->poison2Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison2Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Poison2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison3Chance
									rule->poison3Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison3Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Poison3Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison4Chance
									rule->poison4Chance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison4Chance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"Poison4Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PoisonAddChance
									rule->poisonAdditionalChance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poisonAdditionalChance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"PoisonAddChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PoisonsTierAdjust
									rule->poisonTierAdjust = -1;
									try {
										rule->poisonTierAdjust = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										//logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"PoisonsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										//logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"PoisonsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}

									// now comes FoodChance
									rule->foodChance = Utility::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->foodChance.size() != chancearraysize) {
										//logwarn("[Settings] [LoadDistrRules] fiels \"FoodChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}

									// get strings for the properties
									rule->assocObjects = splits->at(splitindex);
									splitindex++;
									rule->potionProperties = splits->at(splitindex);
									splitindex++;
									rule->fortifyproperties = splits->at(splitindex);
									splitindex++;
									rule->poisonProperties = splits->at(splitindex);
									splitindex++;
									rule->foodProperties = splits->at(splitindex);
									splitindex++;

									if (splits->at(splitindex) == "1")
										rule->allowMixed = true;
									else
										rule->allowMixed = false;
									splitindex++;
									if (splits->at(splitindex) == "1")
										rule->styleScaling = true;
									else
										rule->styleScaling = false;
									splitindex++;

									bool error = false;
									int total = 0;

									// parse the associated objects
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> objects = Utility::ParseAssocObjects(rule->assocObjects, error, file, tmp, total);

									// parse the item properties
									std::vector<std::tuple<AlchemicEffect, float>> potioneffects = Utility::ParseAlchemyEffects(rule->potionProperties, error);
									rule->potionDistr = Utility::GetDistribution(potioneffects, RandomRange);
									//LOG1_4("{}[Settings] [LoadDistrRules] PotionDistr:\t{}", Utility::PrintDistribution(rule->potionDistr));
									rule->potionDistrChance = Utility::GetDistribution(potioneffects, RandomRange, true);
									rule->potionEffectMap = Utility::UnifyEffectMap(potioneffects);
									//LOG1_4("{}[Settings] [LoadDistrRules] PotionEffMap:\t{}", Utility::PrintEffectMap(rule->potionEffectMap));
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} potion effects", rule->ruleName, rule->potionDistr.size());
									rule->validPotions = Utility::SumAlchemyEffects(rule->potionDistr, true);
									std::vector<std::tuple<AlchemicEffect, float>> poisoneffects = Utility::ParseAlchemyEffects(rule->poisonProperties, error);
									rule->poisonDistr = Utility::GetDistribution(poisoneffects, RandomRange);
									//LOG1_4("{}[Settings] [LoadDistrRules] PoisonDistr:\t{}", Utility::PrintDistribution(rule->poisonDistr));
									rule->poisonDistrChance = Utility::GetDistribution(poisoneffects, RandomRange, true);
									rule->poisonEffectMap = Utility::UnifyEffectMap(poisoneffects);
									//LOG1_4("{}[Settings] [LoadDistrRules] PoisonEffMap:\t{}", Utility::PrintEffectMap(rule->poisonEffectMap));
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} poison effects", rule->ruleName, rule->poisonDistr.size());
									rule->validPoisons = Utility::SumAlchemyEffects(rule->poisonDistr, true);
									std::vector<std::tuple<AlchemicEffect, float>> fortifyeffects = Utility::ParseAlchemyEffects(rule->fortifyproperties, error);
									rule->fortifyDistr = Utility::GetDistribution(fortifyeffects, RandomRange);
									//LOG1_4("{}[Settings] [LoadDistrRules] FortifyDistr:\t{}", Utility::PrintDistribution(rule->fortifyDistr));
									rule->fortifyDistrChance = Utility::GetDistribution(fortifyeffects, RandomRange, true);
									rule->fortifyEffectMap = Utility::UnifyEffectMap(fortifyeffects);
									//LOG1_4("{}[Settings] [LoadDistrRules] FortifyEffMap:\t{}", Utility::PrintEffectMap(rule->fortifyEffectMap));
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} fortify potion effects", rule->ruleName, rule->fortifyDistr.size());
									rule->validFortifyPotions = Utility::SumAlchemyEffects(rule->fortifyDistr, true);
									std::vector<std::tuple<AlchemicEffect, float>> foodeffects = Utility::ParseAlchemyEffects(rule->foodProperties, error);
									rule->foodDistr = Utility::GetDistribution(foodeffects, RandomRange);
									//LOG1_4("{}[Settings] [LoadDistrRules] FoodDistr:\t{}", Utility::PrintDistribution(rule->foodDistr));
									rule->foodDistrChance = Utility::GetDistribution(foodeffects, RandomRange, true);
									rule->foodEffectMap = Utility::UnifyEffectMap(foodeffects);
									//LOG1_4("{}[Settings] [LoadDistrRules] FoodEffMap:\t{}", Utility::PrintEffectMap(rule->foodEffectMap));
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} food effects", rule->ruleName, rule->foodDistr.size());
									rule->validFood = Utility::SumAlchemyEffects(rule->foodDistr, true);

									std::pair<int, Distribution::Rule*> tmptuple = { rule->rulePriority, rule };

									// assign rules to search parameters
									//LOGLE2_2("[Settings] [LoadDistrRules] rule {} contains {} associated objects", rule->ruleName, objects.size());
									for (int i = 0; i < objects.size(); i++) {
										switch (std::get<0>(objects[i])) {
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kCombatStyle:
										case Distribution::AssocType::kClass:
										case Distribution::AssocType::kRace:
										case Distribution::AssocType::kKeyword:
											if (auto item = Distribution::_assocMap.find(std::get<1>(objects[i])); item != Distribution::_assocMap.end()) {
												if (std::get<1>(item->second)->rulePriority < rule->rulePriority)
													Distribution::_assocMap.insert_or_assign(std::get<1>(objects[i]), tmptuple);
											} else {
												Distribution::_assocMap.insert_or_assign(std::get<1>(objects[i]), tmptuple);
											}
											break;
										case Distribution::AssocType::kNPC:
										case Distribution::AssocType::kActor:
											if (auto item = Distribution::_npcMap.find(std::get<1>(objects[i])); item != Distribution::_npcMap.end()) {
												if (item->second->rulePriority < rule->rulePriority)
													Distribution::_npcMap.insert_or_assign(std::get<1>(objects[i]), rule);
											} else {
												Distribution::_npcMap.insert_or_assign(std::get<1>(objects[i]), rule);
											}
											break;
										}
									}
									// add rule to the list of rules and we are finished! probably.
									Distribution::_rules.push_back(rule);
									if (rule->ruleName == DefaultRuleName && (Distribution::defaultRule == nullptr ||
																				 rule->rulePriority > Distribution::defaultRule->rulePriority))
										Distribution::defaultRule = rule;
									delete splits;
									//LOGLE1_2("[Settings] [LoadDistrRules] rule {} successfully loaded.", rule->ruleName);
								}
								break;
							default:
								//logwarn("[Settings] [LoadDistrRules] Rule type does not exist for ruleversion 2. file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								break;
							}
						}
						break;
					default:
						//logwarn("[Settings] [LoadDistrRules] Rule version does not exist. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						break;
					}
				}
			} else {
				logger::warn("[Settings] [LoadDistrRules] file {} couldn't be read successfully", file);
			}

		} catch (std::exception&) {
			logger::warn("[Settings] [LoadDistrRules] file {} couldn't be read successfully due to an error", file);
		}
	}

	// create default rule if there is none
	if (Distribution::defaultRule == nullptr) {
		Distribution::defaultRule = new Distribution::Rule(1 /*version*/, 1 /*type*/, DefaultRuleName, INT_MIN + 1 /*rulePriority*/, true /*allowMixed*/, true /*styleScaling*/, 5 /*maxPotions*/, std::vector<int>{ 30, 40, 50, 60, 70 } /*potion1Chance*/,
			std::vector<int>{ 20, 30, 40, 50, 60 } /*potion2Chance*/, std::vector<int>{ 10, 20, 30, 40, 50 } /*potion3Chance*/, std::vector<int>{ 5, 15, 25, 35, 45 } /*potion4Chance*/, std::vector<int>{ 0, 10, 20, 30, 40 } /*potionAddChance*/, 0 /*potionTierAdjust*/,
			5 /*maxFortify*/, std::vector<int>{ 30, 40, 50, 60, 70 } /*fortify1Chance*/, std::vector<int>{ 30, 40, 50, 60, 70 } /*fortify2Chance*/, std::vector<int>{ 20, 30, 40, 50, 60 } /*fortify3Chance*/, std::vector<int>{ 10, 20, 30, 40, 50 } /*fortify4Chance*/, std::vector<int>{ 5, 10, 15, 20, 25 } /*fortifyAddChance*/, 0 /*fortifyTierAdjust*/,
			5 /*maxPoisons*/, std::vector<int>{ 30, 35, 40, 45, 50 } /*poison1Chance*/, std::vector<int>{ 20, 25, 30, 35, 40 } /*poison2Chance*/, std::vector<int>{ 10, 15, 20, 25, 30 } /*poison3Chance*/, std::vector<int>{ 5, 10, 15, 20, 25 } /*poison4Chance*/,
			std::vector<int>{ 0, 5, 10, 15, 20 } /*poisonAddChance*/, 0 /*poisonTierAdjust*/,
			std::vector<int>{ 70, 80, 90, 100, 100 } /*foodChance*/,
			Distribution::GetVector(RandomRange, AlchemicEffect::kAnyPotion) /*potionDistr*/,
			Distribution::GetVector(RandomRange, AlchemicEffect::kAnyPoison) /*poisonDistr*/,
			Distribution::GetVector(RandomRange, AlchemicEffect::kAnyFortify) /*fortifyDistr*/,
			Distribution::GetVector(RandomRange, AlchemicEffect::kAnyFood) /*foodDistr*/,
			AlchemicEffect::kAnyPotion | AlchemicEffect::kCustom /*validPotions*/,
			AlchemicEffect::kAnyPoison | AlchemicEffect::kCustom /*validPoisons*/,
			AlchemicEffect::kAnyFortify | AlchemicEffect::kCustom /*validFortifyPotions*/,
			AlchemicEffect::kAnyFood | AlchemicEffect::kCustom /*validFood*/);
	}
	if (Distribution::defaultCustomRule == nullptr) {
		Distribution::defaultCustomRule = new Distribution::Rule(1 /*version*/, 1 /*type*/, DefaultRuleName, INT_MIN + 1 /*rulePriority*/, true /*allowMixed*/, true /*styleScaling*/, 5 /*maxPotions*/, std::vector<int>{ 30, 40, 50, 60, 70 } /*potion1Chance*/,
			std::vector<int>{ 20, 30, 40, 50, 60 } /*potion2Chance*/, std::vector<int>{ 10, 20, 30, 40, 50 } /*potion3Chance*/, std::vector<int>{ 5, 15, 25, 35, 45 } /*potion4Chance*/, std::vector<int>{ 0, 10, 20, 30, 40 } /*potionAddChance*/, 0 /*potionTierAdjust*/,
			5 /*maxFortify*/, std::vector<int>{ 30, 40, 50, 60, 70 } /*fortify1Chance*/, std::vector<int>{ 30, 40, 50, 60, 70 } /*fortify2Chance*/, std::vector<int>{ 20, 30, 40, 50, 60 } /*fortify3Chance*/, std::vector<int>{ 10, 20, 30, 40, 50 } /*fortify4Chance*/, std::vector<int>{ 5, 10, 15, 20, 25 } /*fortifyAddChance*/, 0 /*fortifyTierAdjust*/,
			5 /*maxPoisons*/, std::vector<int>{ 30, 35, 40, 45, 50 } /*poison1Chance*/, std::vector<int>{ 20, 25, 30, 35, 40 } /*poison2Chance*/, std::vector<int>{ 10, 15, 20, 25, 30 } /*poison3Chance*/, std::vector<int>{ 5, 10, 15, 20, 25 } /*poison4Chance*/,
			std::vector<int>{ 0, 5, 10, 15, 20 } /*poisonAddChance*/, 0 /*poisonTierAdjust*/,
			std::vector<int>{ 70, 80, 90, 100, 100 } /*foodChance*/,
			std::vector<std::tuple<int, AlchemicEffect>>{} /*potionDistr*/,
			std::vector<std::tuple<int, AlchemicEffect>>{} /*poisonDistr*/,
			std::vector<std::tuple<int, AlchemicEffect>>{} /*fortifyDistr*/,
			std::vector<std::tuple<int, AlchemicEffect>>{} /*foodDistr*/,
			AlchemicEffect::kCustom /*validPotions*/,
			AlchemicEffect::kCustom /*validPoisons*/,
			AlchemicEffect::kCustom /*validFortifyPotions*/,
			AlchemicEffect::kCustom /*validFood*/);
	}

	if (copyrules.size() > 0) {
		for (auto& cpy : copyrules) {
			auto splits = std::get<0>(cpy);
			if (splits->size() != 5) {
				logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 5. file: {}, rule:\"{}\", fields: {}", std::get<1>(cpy), std::get<2>(cpy), splits->size());
				continue;
			}
			std::string name = (splits)->at(2);
			std::string newname = (splits)->at(3);
			Distribution::Rule* rule = Distribution::FindRule(name);
			Distribution::Rule* newrule = rule->Clone();
			newrule->ruleName = newname;
			int prio = INT_MIN;
			try {
				prio = std::stoi(splits->at(4));
			} catch (std::out_of_range&) {
				logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"RulePrio\". file: {}, rule:\"{}\"", std::get<1>(cpy), std::get<2>(cpy));
				prio = INT_MIN;
			} catch (std::invalid_argument&) {
				logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RulePrio\". file: {}, rule:\"{}\"", std::get<1>(cpy), std::get<2>(cpy));
				prio = INT_MIN;
			}
			if (prio != INT_MIN)
				newrule->rulePriority = prio;
			Distribution::_rules.push_back(newrule);
			if (newname == DefaultRuleName && (Distribution::defaultRule == nullptr ||
												  newrule->rulePriority > Distribution::defaultRule->rulePriority))
				Distribution::defaultRule = newrule;
			delete splits;
			//LOGE1_2("[Settings] [LoadDistrRules] rule {} successfully coinialised.", newrule->ruleName);
		}
	}

	// and now for the attachement rules.
	// 
	// vector of splits, filename and line
	//std::vector<std::tuple<std::vector<std::string>*, std::string, std::string>> attachments;
	if (attachments.size() > 0) {
		std::string name;
		for (auto& a : attachments) {
			// first two splits are version and type which are already confirmed, so just process the last two.
			// 3rd split is the name of the rule, which the objects in the 4th or 5th split are attached to
			if (std::get<0>(a)->size() == 4 || std::get<0>(a)->size() == 5) {
				{
					// valid rule
					name = (std::get<0>(a))->at(2);
					Distribution::Rule* rule = Distribution::FindRule(name);
					if (rule == nullptr) {
						logger::warn("[Settings] [LoadDistrRules] rule not found. file: {}, rule:\"{}\"", std::get<1>(a), std::get<2>(a));
						continue;  // rule doesn't exist, evaluate next attachment
					}

					// get rule priority and index for assoc objects
					int prio = INT_MIN;
					int associdx = 3;
					// get priority if we have 5 fields, otherwise use rule priority
					if (std::get<0>(a)->size() == 4)
						prio = rule->rulePriority;
					else {
						try {
							prio = std::stoi((std::get<0>(a))->at(3));
							associdx = 4;
						} catch (std::out_of_range&) {
							logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"RulePrio\". file: {}, rule:\"{}\"", std::get<1>(a), std::get<2>(a));
							continue;
						} catch (std::invalid_argument&) {
							logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RulePrio\". file: {}, rule:\"{}\"", std::get<1>(a), std::get<2>(a));
							continue;
						}
					}


					// parse the associated objects
					bool error = false;
					int total = 0;
					std::vector<std::tuple<Distribution::AssocType, RE::FormID>> objects = Utility::ParseAssocObjects((std::get<0>(a)->at(3)), error, std::get<1>(a), std::get<2>(a), total);

					std::pair<int, Distribution::Rule*> tmptuple = { prio, rule };
					// assign rules to search parameters
					bool attach = false; // loop intern
					int oldprio = INT_MIN;
					for (int i = 0; i < objects.size(); i++) {
						switch (std::get<0>(objects[i])) {
						case Distribution::AssocType::kFaction:
						case Distribution::AssocType::kKeyword:
						case Distribution::AssocType::kRace:
						case Distribution::AssocType::kClass:
						case Distribution::AssocType::kCombatStyle:
							if (auto item = Distribution::_assocMap.find(std::get<1>(objects[i])); item != Distribution::_assocMap.end()) {
								if ((oldprio = std::get<1>(item->second)->rulePriority) < rule->rulePriority) {
									Distribution::_assocMap.insert_or_assign(std::get<1>(objects[i]), tmptuple);
									attach = false;
								}
							} else {
								Distribution::_assocMap.insert_or_assign(std::get<1>(objects[i]), tmptuple);
								attach = true;
							}
							break;
						case Distribution::AssocType::kNPC:
						case Distribution::AssocType::kActor:
							if (auto item = Distribution::_npcMap.find(std::get<1>(objects[i])); item != Distribution::_npcMap.end()) {
								if ((oldprio = item->second->rulePriority) < rule->rulePriority) {
									Distribution::_npcMap.insert_or_assign(std::get<1>(objects[i]), rule);
									attach = false;
								}
							} else {
								Distribution::_npcMap.insert_or_assign(std::get<1>(objects[i]), rule);
								attach = true;
							}
							break;
						}
						if (Logging::EnableLog) {
							switch (std::get<0>(objects[i])) {
							case Distribution::AssocType::kFaction:
								if (attach) {
									//LOGE3_2("[Settings] [LoadDistrRules] attached Faction {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else 
									//LOGE5_2("[Settings] [LoadDistrRules] updated Faction {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kKeyword:
								if (attach) {
									//LOGE3_2("[Settings] [LoadDistrRules] attached Keyword {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									//LOGE5_2("[Settings] [LoadDistrRules] updated Keyword {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kRace:
								if (attach) {
									//LOGE3_2("[Settings] [LoadDistrRules] attached Race {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									//LOGE5_2("[Settings] [LoadDistrRules] updated Race {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kClass:
								if (attach) {
									//LOGE3_2("[Settings] [LoadDistrRules] attached Class {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									//LOGE5_2("[Settings] [LoadDistrRules] updated Class {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kCombatStyle:
								if (attach) {
									//LOGE3_2("[Settings] [LoadDistrRules] attached CombatStyle {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									//LOGE5_2("[Settings] [LoadDistrRules] updated CombatStyle {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kNPC:
							case Distribution::AssocType::kActor:
								if (attach) {
									//LOGE3_2("[Settings] [LoadDistrRules] attached Actor {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									LOGE5_2("[Settings] [LoadDistrRules] updated Actor {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							}
						}

					}
				}
			} else {
				// rule invalid
				logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 4 or 5. file: {}, rule:\"{}\"", std::get<1>(a), std::get<2>(a));
				// delet splits since we don't need it anymore
				delete std::get<0>(a);
			}
		}
	}

	/// EXCLUDE ITEMS

	// handle standard exclusions
	RE::TESForm* tmp = nullptr;
	
	// MQ201Drink (don't give quest items out)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00036D53, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// Unknown Potion with unknown effect (in-game type)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0005661F, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// Kordirs skooma: its probably kordirs
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00057A7B, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// Stallion's potion: its probably stallions
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0005566A, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// DB03Poison (quest item)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00058CFB, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// DA16TorporPotion (quest item)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00005F6DF, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// IVDGhostPotion (special item)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000663E1, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// DummyPotion
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0006A07E, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// TG00FalmerBlood - Falmer Blood Elixier
	// DA14Water - Holy Water
	// TGTQ02BalmoraBlue (quest item)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000DC172, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// CW01BWraithPoison (quest item)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000E2D3D, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// Blades Potion: probably esberns
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000E6DF5, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// MS14WineAltoA: probably jessica's 
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000F257E, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// White Phial
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00102019, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// White Phial
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010201A, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// White Phial
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010201B, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// White Phial
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010201C, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// White Phial
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010201D, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// White Phial
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010201E, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// DA03FoodMammothMeat (quest item)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010211A, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// Mq101JuniperMead (quest item)
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00107A8A, "", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());

	// DLC1FoodSoulHusk
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x014DC4, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());
	// DLC1FoodSoulHuskExtract
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x015A1E, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedItems.insert(tmp->GetFormID());


	/// EXCLUDE SUMMONS

	// DA14Summoned
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0001F3AA, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFlame
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000204C0, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFrost
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000204C1, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachStorm
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000204C2, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFlamePotent
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0004E940, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFrostPotent
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0004E943, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachStormPotent
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0004E944, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// EncSummonFamiliar
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000640B5, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// MGArnielSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0006A152, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonPhantom
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00072310, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// dunSummonedSkeleton01Missile
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0007503C, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFlameThrall
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0007E87D, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFrostThrall
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0007E87E, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachStormThrall
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0007E87F, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonFireStorm
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000877EB, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonFelldir
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000923F9, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonGormlaith
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000923FA, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonHakon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000923FB, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// MGRDremoraSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00099F2F, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonFlamingThrall
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0009CE28, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// dunSummonedCreature
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000CC5A2, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFlameThrallPotent
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000CDECC, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFrostThrallPotent
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000CDECD, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachStormThrallPotent
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000CDECE, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// HowlSummonWolf
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000CF79E, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// dunFortSnowhawkSummonedSkeleton01
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000D8D95, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// dunSummonedSkeleton01Melee1HShield
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x000F90BC, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonEncDremoraLord
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010DDEE, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DA14DremoraSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010E38B, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFrostNPC
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010EE43, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachStormNPC
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010EE45, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachFrostNPCPotent
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010EE46, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// SummonAtronachStormNPCPotent
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0010EE47, "", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1SoulCairnWrathmanSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0045B4, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1SoulCairnMistmanSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0045B7, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1SoulCairnBonemanSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0045B9, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1HowlSummonIceWolf
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x008A6C, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1HowlSummonWerewolf
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x008A6D, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC01SoulCairnHorseSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00BDD0, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1VQ05BonemanSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x00BFF0, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1EncUndeadSummon1
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x01A16A, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1EncUndeadSummon2
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x01A16B, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC1EncUndeadSummon3
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x01A16C, "Dawnguard.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2SummonAshGuardian
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0177B6, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2SummonAshSpawn01
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x01CDF8, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2SummonAshGuardianNeloth
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x01DBDC, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2SummonTrollFrost
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x01DFA1, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2SummonSeeker
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x01EEC9, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2dunKarstaagSummon
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x024811, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2SummonSeekerHigh
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x030CDE, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2SummonWerebear
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x0322B3, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
	// DLC2dunKarstaagIceWraithSummoned
	if ((tmp = UtilityAlch::GetTESForm(datahandler, 0x034B5A, "Dragonborn.esm", "")) != nullptr)
		Distribution::_excludedNPCs.insert(tmp->GetFormID());
		
	// EXCLUDED FACTIONS


	// template:
	//if ((tmp = UtilityAlch::GetTESForm(datahandler, 0, "", "")) != nullptr)
	//	Distribution::_excludedItems.insert(tmp->GetFormID());

	Distribution::initialised = true;

	if (Logging::EnableLog) {
		logger::info("[Settings] [LoadDistrRules] Number of Rules: {}", Distribution::rules()->size());
		logger::info("[Settings] [LoadDistrRules] Number of NPCs: {}", Distribution::npcMap()->size());
		logger::info("[Settings] [LoadDistrRules] Buckets of NPCs: {}", Distribution::npcMap()->bucket_count());
		logger::info("[Settings] [LoadDistrRules] Number of Associations: {}", Distribution::assocMap()->size());
		logger::info("[Settings] [LoadDistrRules] Buckets of Associations: {}", Distribution::assocMap()->bucket_count());
		logger::info("[Settings] [LoadDistrRules] Number of Bosses: {}", Distribution::bosses()->size());
		logger::info("[Settings] [LoadDistrRules] Buckets of Bosses: {}", Distribution::bosses()->bucket_count());
		logger::info("[Settings] [LoadDistrRules] Number of Excluded NPCs: {}", Distribution::excludedNPCs()->size());
		logger::info("[Settings] [LoadDistrRules] Buckets of Excluded NPCs: {}", Distribution::excludedNPCs()->bucket_count());
		logger::info("[Settings] [LoadDistrRules] Number of Excluded Associations: {}", Distribution::excludedAssoc()->size());
		logger::info("[Settings] [LoadDistrRules] Buckets of Excluded Associations: {}", Distribution::excludedAssoc()->bucket_count());
		logger::info("[Settings] [LoadDistrRules] Number of Excluded Items: {}", Distribution::excludedItems()->size());
		logger::info("[Settings] [LoadDistrRules] Buckets of Excluded Items: {}", Distribution::excludedItems()->bucket_count());
		logger::info("[Settings] [LoadDistrRules] Number of Baseline Exclusions: {}", Distribution::baselineExclusions()->size());
		logger::info("[Settings] [LoadDistrRules] Buckets of Baseline Exclusions: {}", Distribution::baselineExclusions()->bucket_count());
		/*for (int i = 0; i < Distribution::_rules.size(); i++) {
			logger::info("rule {} pointer {}", i, UtilityAlch::GetHex((uintptr_t)Distribution::_rules[i]));
		}
		auto iter = Distribution::_assocMap.begin();
		while (iter != Distribution::_assocMap.end()) {
			logger::info("assoc\t{}\trule\t{}", UtilityAlch::GetHex(iter->first), UtilityAlch::GetHex((uintptr_t)(std::get<1>(iter->second))));
			iter++;
		}*/
	}
}

std::tuple<std::shared_ptr<DiseaseStage>, uint16_t> LoadDiseaseStage(std::vector<std::string>* splits, std::string file, std::string tmp)
{
	int splitindex = 0;
	if (splits->size() < 69)  // if there are too few stages
	{
		logger::warn("[Settings] [LoadDiseaseStage] Not a rule. Expected {} fields, found {}. file: {}, rule:\"{}\"", 69, splits->size(), file, tmp);
		return {};
	}

	std::shared_ptr<DiseaseStage> stg = std::make_shared<DiseaseStage>();

	// get id
	uint16_t stageid;
	try {
		stageid = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDiseaseStage] out-of-range expection in field \"StageID\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDiseaseStage] invalid-argument expection in field \"StageID\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// get specifier
	stg->_specifier = splits->at(splitindex);
	splitindex++;

	// AdvancementThreshold
	try {
		stg->_advancementThreshold = std::stof(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDiseaseStage] out-of-range expection in field \"AdvancementThreshold\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDiseaseStage] invalid-argument expection in field \"AdvancementThreshold\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// AdvancementTime
	try {
		stg->_advancementTime = std::stof(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"AdvancementTime\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDiseaseStage] invalid-argument expection in field \"AdvancementTime\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// Infectivity
	try {
		stg->_infectivity = static_cast<Infectivity>(std::stoi(splits->at(splitindex)));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"Infectivity\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDiseaseStage] invalid-argument expection in field \"Infectivity\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// SpellFormID
	RE::FormID spellid;
	if (splits->at(splitindex) == "") {
		spellid = 0;
		splitindex++;
	}
	else
		try {
			spellid = std::stoull(splits->at(splitindex), nullptr, 16);
			splitindex++;
		} catch (std::out_of_range&) {
			logwarn("[Settings] [LoadDisease] out-of-range expection in field \"SpellFormID\". file: {}, rule:\"{}\"", file, tmp);
			return {};
		} catch (std::invalid_argument&) {
			logwarn("[Settings] [LoadDiseaseStage] invalid-argument expection in field \"SpellFormID\". file: {}, rule:\"{}\"", file, tmp);
			return {};
		}

	// pluginname
	std::string pluginname = splits->at(splitindex);
	splitindex++;
	if (Utility::ToLower(pluginname) == "settings")
		pluginname = Settings::PluginName;

	stg->effect = Data::GetSingleton()->FindSpell(spellid, pluginname);

	auto ParseEffects = [&splits, &splitindex, &file, &tmp]() {
		DiseaseEffect eff;
		if (splits->at(splitindex) == "") {
			eff = DiseaseEffect::kNone;
			splitindex++;
		}
		else
			try {
				eff = static_cast<DiseaseEffect>(std::stoi(splits->at(splitindex)));
				splitindex++;
			} catch (std::out_of_range&) {
				return std::pair<DiseaseEffect, Magnitude>{ DiseaseEffect::kNone, 0.0f };
			} catch (std::invalid_argument&) {
				return std::pair<DiseaseEffect, Magnitude>{ DiseaseEffect::kNone, 0.0f };
			}
		Magnitude mag;
		if (splits->at(splitindex) == "") {
			mag = 0.0f;
			splitindex++;
		}
		else
			try {
				mag = std::stof(splits->at(splitindex));
				splitindex++;
			} catch (std::out_of_range&) {
				return std::pair<DiseaseEffect, Magnitude>{ DiseaseEffect::kNone, 0.0f };
			} catch (std::invalid_argument&) {
				return std::pair<DiseaseEffect, Magnitude>{ DiseaseEffect::kNone, 0.0f };
			}
		return std::pair<DiseaseEffect, Magnitude>(eff, mag);
	};

	auto ParseSpreading = [&splits, &splitindex, &file, &tmp]() {
		float chance, points;
		if (splits->at(splitindex) == "") {
			chance = 0.0f;
			splitindex++;
		}
		else
			try {
				chance = std::stof(splits->at(splitindex));
				splitindex++;
			} catch (std::out_of_range&) {
				return std::pair<float, float>{ 0.0f, 0.0f };
			} catch (std::invalid_argument&) {
				return std::pair<float, float>{ 0.0f, 0.0f };
			}
		if (splits->at(splitindex) == "") {
			points = 0.0f;
			splitindex++;
		}
		else
			try {
				points = std::stof(splits->at(splitindex));
				splitindex++;
			} catch (std::out_of_range&) {
				return std::pair<float, float>{ 0.0f, 0.0f };
			} catch (std::invalid_argument&) {
				return std::pair<float, float>{ 0.0f, 0.0f };
			}
		loginfo("[ParseSpreading] {}, {}", chance, points);
		return std::pair<float, float>(chance, points);
	};

	auto ParsePoints = [&splits, &splitindex, &file, &tmp]() {
		float f;
		if (splits->at(splitindex) == "") {
			f = 0.0f;
			splitindex++;
		}
		else
			try {
				f = std::stof(splits->at(splitindex));
				splitindex++;
			} catch (std::out_of_range&) {
				return 0.0f;
			} catch (std::invalid_argument&) {
				return 0.0f;
			}
		return f;
	};

	// Effect1
	stg->_effects.push_back(ParseEffects());
	// Effect2
	stg->_effects.push_back(ParseEffects());
	// Effect3
	stg->_effects.push_back(ParseEffects());
	// Effect4
	stg->_effects.push_back(ParseEffects());
	// Effect5
	stg->_effects.push_back(ParseEffects());
	// Effect6
	stg->_effects.push_back(ParseEffects());
	// Effect7
	stg->_effects.push_back(ParseEffects());
	// Effect8
	stg->_effects.push_back(ParseEffects());
	// Effect9
	stg->_effects.push_back(ParseEffects());

	// OnHitMelee
	stg->_spreading[Spreading::kOnHitMelee] = ParseSpreading();
	// kOnHitRanged
	stg->_spreading[Spreading::kOnHitRanged] = ParseSpreading();
	// kOnHitH2H
	stg->_spreading[Spreading::kOnHitH2H] = ParseSpreading();
	// kGetHitMelee
	stg->_spreading[Spreading::kGetHitMelee] = ParseSpreading();
	// kGetHitH2H
	stg->_spreading[Spreading::kGetHitH2H] = ParseSpreading();
	// kAir
	stg->_spreading[Spreading::kAir] = ParseSpreading();
	// kParticle
	stg->_spreading[Spreading::kParticle] = ParseSpreading();
	// kIntenseCold
	stg->_spreading[Spreading::kIntenseCold] = ParseSpreading();
	// kIntenseHeat
	stg->_spreading[Spreading::kIntenseHeat] = ParseSpreading();
	// kInAshland
	stg->_spreading[Spreading::kInAshland] = ParseSpreading();
	// kInSwamp
	stg->_spreading[Spreading::kInSwamp] = ParseSpreading();
	// kInDessert
	stg->_spreading[Spreading::kInDessert] = ParseSpreading();
	// kInAshstorm
	stg->_spreading[Spreading::kInAshstorm] = ParseSpreading();
	// kInSandstorm
	stg->_spreading[Spreading::kInSandstorm] = ParseSpreading();
	// kInBlizzard
	stg->_spreading[Spreading::kInBlizzard] = ParseSpreading();
	// kInRain
	stg->_spreading[Spreading::kInRain] = ParseSpreading();
	// kIsWindy
	stg->_spreading[Spreading::kIsWindy] = ParseSpreading();
	// kIsStormy
	stg->_spreading[Spreading::kIsStormy] = ParseSpreading();
	// kIsCold
	stg->_spreading[Spreading::kIsCold] = ParseSpreading();
	// kIsHeat
	stg->_spreading[Spreading::kIsHeat] = ParseSpreading();
	// kExtremeConditions
	stg->_spreading[Spreading::kExtremeConditions] = ParseSpreading();
	// kActionPhysical
	stg->_spreading[Spreading::kActionPhysical] = std::pair<float, float>{ 100.0f, ParsePoints() };
	// kActionMagical
	stg->_spreading[Spreading::kActionMagical] = std::pair<float, float>{ 100.0f, ParsePoints() };

	return { stg, stageid };
}

std::tuple<std::shared_ptr<Disease>, uint16_t, uint16_t, std::vector<uint16_t>> LoadDisease(std::vector<std::string>* splits, std::string file, std::string tmp)
{
	int splitindex = 0; 
	if (splits->size() < 18) // if there are too few stages
	{
		logger::warn("[Settings] [LoadDisease] Not a rule. Expected {} fields, found {}. file: {}, rule:\"{}\"", 18, splits->size(), file, tmp);
		return {};
	}

	std::shared_ptr<Disease> dis = std::make_shared<Disease>();

	// get name
	dis->_name = splits->at(splitindex);
	splitindex++;

	// get disease
	uint16_t diseaseind = 0;
	try
	{
		diseaseind = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"Disease\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"Disease\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}
	dis->_disease = static_cast<Diseases::Disease>(diseaseind);

	// get disease type
	try
	{
		dis->_type = static_cast<DiseaseType>(std::stoi(splits->at(splitindex), nullptr, 16));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"DiseaseType\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"DiseaseType\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// get valid permanent modifiers
	try
	{
		dis->_validModifiers = std::stoul(splits->at(splitindex), nullptr, 16);
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"PermanentModifiers\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"PermanentModifiers\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// immunity time
	try
	{
		dis->immunityTime = std::stof(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"ImmunityTime\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"ImmunityTime\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// base progression points
	try
	{
		dis->_baseProgressionPoints = std::stof(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"BaseProgressionPoints\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"BaseProgressionPoints\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// base infection reduction points
	try
	{
		dis->_baseInfectionReductionPoints = std::stof(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"BaseInfectionReductionPoints\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"BaseInfectionReductionPoints\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// base infection chance
	try
	{
		dis->_baseInfectionChance = std::stof(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"BaseInfectionChance\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"BaseInfectionChance\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// numstages
	try
	{
		dis->_numstages = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"NumStages\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"NumStages\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// end events
	try
	{
		dis->endevents = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"EndEvents\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"EndEvents\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// get formid
	RE::FormID endeffect;
	if (splits->at(splitindex) == "") {
		endeffect = 0;
		splitindex++;
	}
	else
		try
		{
			endeffect = std::stoul(splits->at(splitindex), nullptr, 16);
			splitindex++;
		} catch (std::out_of_range&) {
			logwarn("[Settings] [LoadDisease] out-of-range expection in field \"EndEffect\". file: {}, rule:\"{}\"", file, tmp);
			return {};
		} catch (std::invalid_argument&) {
			logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"EndEffect\". file: {}, rule:\"{}\"", file, tmp);
			return {};
		}
	// get plugin name
	std::string pluginname = splits->at(splitindex);
	splitindex++;
	if (Utility::ToLower(pluginname) == "settings")
		pluginname = Settings::PluginName;

	// get endeffect
	dis->endeffect = Data::GetSingleton()->FindSpell(endeffect, pluginname);

	// get stageinfection
	uint16_t infectionid;
	try
	{
		infectionid = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"StageInfection\" with argument {}. file: {}, rule:\"{}\"", splits->at(splitindex), file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"StageInfection\" with argument {}. file: {}, rule:\"{}\"", splits->at(splitindex), file, tmp);
		return {};
	}

	// get stageincubation
	uint16_t incubationid;
	try
	{
		incubationid = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"StageIncubation\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"StageIncubation\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// get stage1
	uint16_t stage1;
	try {
		stage1 = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"Stage1\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"Stage1\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// get stage2
	uint16_t stage2;
	try {
		stage2 = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"Stage2\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"Stage2\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// get stage3
	uint16_t stage3;
	try {
		stage3 = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"Stage3\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"Stage3\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	// get stage4
	uint16_t stage4;
	try {
		stage4 = std::stoi(splits->at(splitindex));
		splitindex++;
	} catch (std::out_of_range&) {
		logwarn("[Settings] [LoadDisease] out-of-range expection in field \"Stage4\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	} catch (std::invalid_argument&) {
		logwarn("[Settings] [LoadDisease] invalid-argument expection in field \"Stage4\". file: {}, rule:\"{}\"", file, tmp);
		return {};
	}

	return {
		dis, infectionid, incubationid, { stage1, stage2, stage3, stage4 }
	};
}

void Settings::LoadDistrConfig()
{
	LOG_1("{}[Settings] [LoadDistrConfig]");
	// set to false, to avoid other funcions running stuff on our variables
	Distribution::initialised = false;

	// find all stage defining files

	std::vector<std::string> rawfilesstages = { R"(Data\SKSE\Plugins\DiseaseOverhaul_Blight_Stages.csv)", R"(Data\SKSE\Plugins\DiseaseOverhaul_Common_Stages.csv)", R"(Data\SKSE\Plugins\DiseaseOverhaul_Dangerous_Stages.csv)", R"(Data\SKSE\Plugins\DiseaseOverhaul_Extreme_Stages.csv)", R"(Data\SKSE\Plugins\DiseaseOverhaul_Fever_Stages.csv)", R"(Data\SKSE\Plugins\DiseaseOverhaul_Mild_Stages.csv)", R"(Data\SKSE\Plugins\DiseaseOverhaul_Special_Stages.csv)" };

	std::vector<std::string> filesstages;

	for (const auto& file : rawfilesstages) {
		auto entry = std::filesystem::directory_entry(std::filesystem::path(file));
		if (entry.exists() && !entry.path().empty()) {
			filesstages.push_back(entry.path().string());
		}
	}

	// find all disease defining files

	std::vector<std::string> rawfilesdiseases = { R"(Data\SKSE\Plugins\DiseaseOverhaul_Diseases.csv)" };

	std::vector<std::string> filesdiseases;

	for (const auto& file : rawfilesdiseases) {
		auto entry = std::filesystem::directory_entry(std::filesystem::path(file));
		if (entry.exists() && !entry.path().empty()) {
			filesdiseases.push_back(entry.path().string());
		}
	}

	// read the disease stages

	int linecount = 0;
	for (std::string file : filesstages) {
		linecount = 0;
		try {
			std::ifstream infile(file);
			if (infile.is_open()) {
				std::string line;
				while (std::getline(infile, line)) {
					linecount++;
					if (linecount == 1)  // skip first line since its the header
						continue;

					std::string tmp = line;
					// we read another line
					// check if its empty or with a comment
					if (line.empty())
						continue;
					// remove leading spaces and tabs
					while (line.length() > 0 && (line[0] == ' ' || line[0] == '\t')) {
						line = line.substr(1, line.length() - 1);
					}
					// check again
					if (line.length() == 0 || line[0] == ';')
						continue;
					// now begin the actual processing
					std::vector<std::string>* splits = new std::vector<std::string>();
					// split the string into parts
					size_t pos = line.find('|');
					while (pos != std::string::npos) {
						splits->push_back(line.substr(0, pos));
						line.erase(0, pos + 1);
						pos = line.find("|");
					}
					if (line.length() != 0)
						splits->push_back(line);
					else
						splits->push_back("");

					// load the disease stage
					auto [stg, id] = LoadDiseaseStage(splits, file, tmp);
					if (stg) {
						// if the stage returned is valid, save it to data
						Data::GetSingleton()->AddDiseaseStage(stg, id);

						LOGLE1_1("[Settings] [LoadDistrRules] loaded DiseaseStage: {}", UtilityAlch::ToString(stg));
						loginfo("DiseaseStage+");
					} else
						loginfo("DiseaseStage");

					delete splits;
				}
			} else {
				logger::warn("[Settings] [LoadDistrConfig] file {} couldn't be read successfully", file);
			}
		} catch (std::exception&) {
			logger::warn("[Settings] [LoadDistrConfig] file {} couldn't be read successfully due to an error", file);
		}
	}

	// read the diseases

	linecount = 0;
	for (std::string file : filesdiseases) {
		linecount = 0;
		try {
			std::ifstream infile(file);
			if (infile.is_open()) {
				std::string line;
				while (std::getline(infile, line)) {
					linecount++;
					if (linecount == 1)  // skip first line since its the header
						continue;

					std::string tmp = line;
					// we read another line
					// check if its empty or with a comment
					if (line.empty())
						continue;
					// remove leading spaces and tabs
					while (line.length() > 0 && (line[0] == ' ' || line[0] == '\t')) {
						line = line.substr(1, line.length() - 1);
					}
					// check again
					if (line.length() == 0 || line[0] == ';')
						continue;
					// now begin the actual processing
					std::vector<std::string>* splits = new std::vector<std::string>();
					// split the string into parts
					size_t pos = line.find('|');
					while (pos != std::string::npos) {
						splits->push_back(line.substr(0, pos));
						line.erase(0, pos + 1);
						pos = line.find("|");
					}
					if (line.length() != 0)
						splits->push_back(line);
					else
						splits->push_back("");

					auto [disease, infectionid, incubationid, stageids] = LoadDisease(splits, file, tmp);
					if (disease) {
						Data::GetSingleton()->InitDisease(disease, infectionid, incubationid, stageids);

						LOGLE1_1("[Settings] [LoadDistrRules] loaded Disease: {}", UtilityAlch::ToString(disease));
						loginfo("Disease+");
					} else
						loginfo("Disease");

					delete splits;
				}
			} else {
				logger::warn("[Settings] [LoadDistrConfig] file {} couldn't be read successfully", file);
			}
		} catch (std::exception&) {
			logger::warn("[Settings] [LoadDistrConfig] file {} couldn't be read successfully due to an error", file);
		}
	}

	std::vector<std::string> files;
	auto constexpr folder = R"(Data\SKSE\Plugins\)";
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini") {
			if (auto path = entry.path().string(); path.rfind("DO_DIST") != std::string::npos) {
				files.push_back(path);
				logger::info("[SETTINGS] [LoadDistrConfig] found Distribution configuration file: {}", entry.path().filename().string());
			}
		}
	}
	if (files.empty()) {
		logger::info("[SETTINGS] [LoadDistrConfig] No Distribution files were found");
	}
	// init datahandler
	auto datahandler = RE::TESDataHandler::GetSingleton();

	std::unordered_map<std::string, std::pair<std::set<RE::FormID>, std::set<std::string> /*further categories*/>> categories;

	std::vector<std::tuple<Diseases::Disease, float /*chance*/, float /*scale*/, std::set<std::string> /*categories*/>> infecrules;

	// vector of splits, filename and line

	// extract the rules from all files
	for (std::string file : files) {
		try {
			std::ifstream infile(file);
			if (infile.is_open()) {
				std::string line;
				while (std::getline(infile, line)) {
					std::string tmp = line;
					// we read another line
					// check if its empty or with a comment
					if (line.empty())
						continue;
					// remove leading spaces and tabs
					while (line.length() > 0 && (line[0] == ' ' || line[0] == '\t')) {
						line = line.substr(1, line.length() - 1);
					}
					// check again
					if (line.length() == 0 || line[0] == ';')
						continue;
					// now begin the actual processing
					std::vector<std::string>* splits = new std::vector<std::string>();
					// split the string into parts
					size_t pos = line.find('|');
					while (pos != std::string::npos) {
						splits->push_back(line.substr(0, pos));
						line.erase(0, pos + 1);
						pos = line.find("|");
					}
					if (line.length())
						splits->push_back(line);
					else
						splits->push_back("");
					int splitindex = 0;
					// check wether we actually have a rule
					if (splits->size() < 3) {  // why 3? Cause first two fields are RuleVersion and RuleType and we don't accept empty rules.
						logger::warn("[Settings] [LoadDistrConfig] Not a rule. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what rule version we have
					int ruleVersion = -1;
					try {
						ruleVersion = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						logger::warn("[Settings] [LoadDistrConfig] out-of-range expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						logger::warn("[Settings] [LoadDistrConfig] invalid-argument expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what kind of rule we have
					int ruleType = -1;
					try {
						ruleType = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						logger::warn("[Settings] [LoadDistrConfig] out-of-range expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						logger::warn("[Settings] [LoadDistrConfig] invalid-argument expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// now we can actually make differences for the different rule version and types
					switch (ruleVersion) {
					case 1:
						{
							switch (ruleType) {
							case 1:  // distribution rule
								// Does not exist for AlchExt
								delete splits;
								break;
							case 2:  // distribution attachement
								// Does not exist for AlchExt
								delete splits;
								break;
							case 3:  // declare boss
								// Does not exist for AlchExt
								delete splits;
								break;
							case 4:  // exclude object
								// Does not exist for AlchExt
								delete splits;
								break;
							case 5:  // exclude baseline
								// Does not exist for AlchExt
								delete splits;
								break;
							case 6:  // copy rule
								// Does not exist for AlchExt
								delete splits;
								break;
							case 7:  // whitelist rule
								// Does not exist for AlchExt
								delete splits;
								break;
							case 8:  // custom object distribution
								{
									if (splits->size() != 4) {
										logger::warn("[Settings] [LoadDistrConfig] rule has wrong number of fields, expected 4. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}

									Distribution::CustomItemStorage* citems = new Distribution::CustomItemStorage();
									// parse associated obj
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> assocobj = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp, total);

									// parse items associated
									assoc = splits->at(splitindex);
									splitindex++;
									error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID, int32_t, CustomItemFlag, int8_t, bool, std::vector<std::tuple<uint64_t, uint32_t, std::string>>, std::vector<std::tuple<uint64_t, uint32_t, std::string>>, bool>> associtm = UtilityAlch::ParseCustomObjects(assoc, error, file, tmp);
									RE::TESForm* tmpf = nullptr;
									RE::TESBoundObject* tmpb = nullptr;
									RE::AlchemyItem* alch = nullptr;
									// parse items first: if there are no items we don't need to do anything further
									for (int i = 0; i < associtm.size(); i++) {
										tmpf = nullptr;
										tmpb = nullptr;
										switch (std::get<0>(associtm[i])) {
										case Distribution::AssocType::kItem:
											{
												tmpf = RE::TESForm::LookupByID(std::get<1>(associtm[i]));
												if (tmpf) {
													tmpb = tmpf->As<RE::TESBoundObject>();
													alch = tmpf->As<RE::AlchemyItem>();
												}
												LOGL1_3("{}[Settings] [LoadDstrRules] Flag converted: {}", static_cast<uint64_t>(std::get<3>(associtm[i])));
												if (tmpb) {
													if (std::get<5>(associtm[i]))
														Distribution::_excludedItems.insert(std::get<2>(associtm[i]));
													switch (std::get<3>(associtm[i])) {
													case CustomItemFlag::Object:
														{
															LOGL_3("{}[Settings] [LoadDstrRules] Path 1");
															CustomItem* cit = new CustomItem();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = tmpb;
															citems->items.push_back(cit);
														}
														break;
													case CustomItemFlag::DeathObject:
														{
															LOGL_3("{}[Settings] [LoadDstrRules] Path 2");

															CustomItem* cit = new CustomItem();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = tmpb;
															citems->death.push_back(cit);
														}
														break;
													case CustomItemFlag::Food:
														LOGL_3("{}[Settings] [LoadDstrRules] Path 3");
														if (alch) {
															CustomItemAlch* cit = new CustomItemAlch();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = alch;
															citems->food.push_back(cit);
														}
														break;
													case CustomItemFlag::Fortify:
														LOGL_3("{}[Settings] [LoadDstrRules] Path 4");
														if (alch) {
															CustomItemAlch* cit = new CustomItemAlch();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = alch;
															citems->fortify.push_back(cit);
														}
														break;
													case CustomItemFlag::Poison:
														LOGL_3("{}[Settings] [LoadDstrRules] Path 5");
														if (alch) {
															CustomItemAlch* cit = new CustomItemAlch();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = alch;
															citems->poisons.push_back(cit);
														}
														break;
													case CustomItemFlag::Potion:
														LOGL_3("{}[Settings] [LoadDstrRules] Path 6");
														if (alch) {
															CustomItemAlch* cit = new CustomItemAlch();
															cit->chance = std::get<4>(associtm[i]);
															cit->conditionsall = std::get<6>(associtm[i]);
															cit->conditionsany = std::get<7>(associtm[i]);
															cit->giveonce = std::get<8>(associtm[i]);
															cit->num = std::get<2>(associtm[i]);
															cit->object = alch;
															citems->potions.push_back(cit);
														}
														break;
													}
												} else {
													LOGLE1_2("[Settings] [LoadDistrRules] custom rule for item {} cannot be applied, due to the item not being an TESBoundObject.", Utility::GetHex(std::get<1>(associtm[i])));
												}
											}
											break;
										}
									}
									if (citems->items.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] rule does not contain any items. file: {}, rule:\"{}\"", file, tmp);
										delete citems;
										continue;
									}

									int cx = 0;
									// now parse associations
									for (int i = 0; i < assocobj.size(); i++) {
										switch (std::get<0>(assocobj[i])) {
										case Distribution::AssocType::kActor:
										case Distribution::AssocType::kNPC:
										case Distribution::AssocType::kClass:
										case Distribution::AssocType::kCombatStyle:
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kKeyword:
										case Distribution::AssocType::kRace:
											citems->assocobjects.insert(std::get<1>(assocobj[i]));
											auto iter = Distribution::_customItems.find(std::get<1>(assocobj[i]));
											if (iter != Distribution::_customItems.end()) {
												std::vector<Distribution::CustomItemStorage*> vec = iter->second;
												vec.push_back(citems);
												Distribution::_customItems.insert_or_assign(std::get<1>(assocobj[i]), vec);
												cx++;
											} else {
												std::vector<Distribution::CustomItemStorage*> vec = { citems };
												Distribution::_customItems.insert_or_assign(std::get<1>(assocobj[i]), vec);
												cx++;
											}
											break;
										}
										if (Logging::EnableLog) {
											if (std::get<0>(assocobj[i]) == Distribution::AssocType::kKeyword) {
											} else if (std::get<0>(assocobj[i]) == Distribution::AssocType::kRace) {
											} else if (std::get<0>(assocobj[i]) == Distribution::AssocType::kFaction) {
											} else if (std::get<0>(assocobj[i]) == Distribution::AssocType::kCombatStyle) {
											} else if (std::get<0>(assocobj[i]) == Distribution::AssocType::kClass) {
											} else if (std::get<0>(assocobj[i]) == Distribution::AssocType::kActor || std::get<0>(assocobj[i]) == Distribution::AssocType::kNPC) {
											}
										}
									}
									if (cx == 0) {
										auto iter = Distribution::_customItems.find(0x0);
										if (iter != Distribution::_customItems.end()) {
											std::vector<Distribution::CustomItemStorage*> vec = iter->second;
											vec.push_back(citems);
											Distribution::_customItems.insert_or_assign(0x0, vec);
											cx++;
										} else {
											std::vector<Distribution::CustomItemStorage*> vec = { citems };
											Distribution::_customItems.insert_or_assign(0x0, vec);
											cx++;
										}
									}

									// since we are done delete splits
									delete splits;
								}
								break;
							case 101:  // disease: define infected
								{
									LOGLE_2("[Settings] [LoadDistrRules] Define infected rule");
									if (splits->size() != 4) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									Diseases::Disease disease = Diseases::kAshChancre;
									try {
										disease = static_cast<Diseases::Disease>(std::stoi(splits->at(splitindex)));
										splitindex++;
									} catch (std::out_of_range&) {
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"Disease\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"Disease\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}

									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kActor:
										case Distribution::AssocType::kNPC:
										case Distribution::AssocType::kClass:
										case Distribution::AssocType::kCombatStyle:
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kKeyword:
										case Distribution::AssocType::kRace:
											{
												auto data = Data::GetSingleton();
												data->diseasesForceAssoc.insert_or_assign(std::get<1>(items[i]), disease);
											}
											break;
										}
									}
									delete splits;
								}
								break;
							case 102:  // disease: define cell properties
								{
									LOGLE_2("[Settings] [LoadDistrRules] Define cell properties");
									if (splits->size() != 4) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									CellTypes::CellType celltype = CellTypes::kNone;
									try {
										celltype = static_cast<CellTypes::CellType>(std::stoi(splits->at(splitindex)));
										splitindex++;
									} catch (std::out_of_range&) {
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"CellType\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"CellType\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}

									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kCell:
											{
												auto data = Data::GetSingleton();
												auto itr = data->cellMap.find(std::get<1>(items[i]));
												if (itr != data->cellMap.end() && itr->second) {
													itr->second->type |= celltype;
												}
											}
											break;
										}
									}
									delete splits;
								}
								break;
							case 103:  // disease: define weather properties
								{
									LOGLE_2("[Settings] [LoadDistrRules] Define weather properties");
									if (splits->size() != 4) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									WeatherTypes::WeatherType weathertype = WeatherTypes::kNone;
									try {
										weathertype = static_cast<WeatherTypes::WeatherType>(std::stoi(splits->at(splitindex)));
										splitindex++;
									} catch (std::out_of_range&) {
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"WeatherType\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"WeatherType\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}

									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kWeather:
											{
												auto data = Data::GetSingleton();
												auto itr = data->weatherMap.find(std::get<1>(items[i]));
												if (itr != data->weatherMap.end() && itr->second) {
													itr->second->type |= weathertype;
												}
											}
											break;
										}
									}
									delete splits;
								}
								break;
							case 104:  // disease: define texture properties
								{
									LOGLE_2("[Settings] [LoadDistrRules] Define texture properties");
									if (splits->size() != 4) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									TextureTypes::TextureType texturetype = TextureTypes::kNone;
									try {
										texturetype = static_cast<TextureTypes::TextureType>(std::stoi(splits->at(splitindex)));
										splitindex++;
									} catch (std::out_of_range&) {
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"TextureType\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"TextureType\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}

									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kTextureSet:
											{
												auto data = Data::GetSingleton();
												auto itr = data->textureMap.find(std::get<1>(items[i]));
												if (itr != data->textureMap.end() && itr->second) {
													itr->second->type |= texturetype;
												}
											}
											break;
										}
									}
									delete splits;
								}
								break;
							case 105:  // disease: define category
								{
									LOGLE_2("[Settings] [LoadDistrRules] Define probable infections");
									if (splits->size() != 5) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 5. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}

									std::string category = Utility::ToLower(splits->at(splitindex));
									splitindex++;
									std::vector<std::string> attached = UtilityAlch::SplitString(Utility::ToLower(splits->at(splitindex)), ',', true);
									splitindex++;
									std::set<std::string> attset;
									for (auto& str : attached)
										attset.insert(str);

									std::set<RE::FormID> ids;

									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp, total);
									LOGLE_2("[Settings] [LoadDistrRules] Define probable infections 0");
									for (int i = 0; i < items.size(); i++) {
										//LOGLE_2("[Settings] [LoadDistrRules] Define probable infections 0.5");
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kActor:
										case Distribution::AssocType::kNPC:
										case Distribution::AssocType::kClass:
										case Distribution::AssocType::kCombatStyle:
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kKeyword:
										case Distribution::AssocType::kRace:
											{
												//LOGLE_2("[Settings] [LoadDistrRules] Define probable infections 2");
												ids.insert(std::get<1>(items[i]));
											}
											break;
										}
									}
									delete splits;

									if (category.empty() == false) {
										auto itr = categories.find(category);
										if (itr != categories.end()) {
											// attach new objects to existing category
											auto [a_ids, a_attached] = itr->second;
											for (auto& id : ids)
												a_ids.insert(id);
											for (auto& cat : attset)
												a_attached.insert(cat);
											categories.insert_or_assign(itr->first, std::pair<std::set<RE::FormID>, std::set<std::string> /*further categories*/>{ a_ids, a_attached });
										} else {
											categories.insert_or_assign(category, std::pair<std::set<RE::FormID>, std::set<std::string> /*further categories*/>{ ids, attset });
										}
									}
								}
								break;
							case 106:  // disease: define infection chance
								{
									LOGLE_2("[Settings] [LoadDistrRules] Define probable infections");
									if (splits->size() != 6) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 6. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									Diseases::Disease disease = Diseases::kAshChancre;
									try {
										disease = static_cast<Diseases::Disease>(std::stoi(splits->at(splitindex)));
										splitindex++;
									} catch (std::out_of_range&) {
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"Disease\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"Disease\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}

									float chance = 0;
									try {
										chance = std::stof(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Chance\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"Chance\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}

									float scale = 0;
									try {
										scale = std::stof(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Scale\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"Scale\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}

									std::vector<std::string> cats = UtilityAlch::SplitString(Utility::ToLower(splits->at(splitindex)), ',', true);
									std::set<std::string> catset;
									for (auto& str : cats)
										catset.insert(str);
									splitindex++;

									if (catset.size() > 0)
										infecrules.push_back(std::tuple<Diseases::Disease, float /*chance*/, float /*scale*/, std::set<std::string> /*categories*/>{ disease, chance, scale, catset });

									delete splits;
								}
								break;
							case 107:  // disease: define shrine
								{
									LOGLE_2("[Settings] [LoadDistrRules] Define texture properties");
									if (splits->size() != 4) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}

									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp, total);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kObjectReference:
											{
												auto data = Data::GetSingleton();
												data->shrines.insert(std::get<1>(items[i]));
											}
											break;
										case Distribution::AssocType::kActivator:
											{
												auto data = Data::GetSingleton();
												data->shrines.insert(std::get<1>(items[i]));
											}
											break;
										}
									}
									delete splits;
								}
								break;
							default:
								logger::warn("[Settings] [LoadDistrConfig] Rule type does not exist. file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								break;
							}
						}
						break;
					default:
						logger::warn("[Settings] [LoadDistrConfig] Rule version does not exist. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						break;
					}
				}
			} else {
				logger::warn("[Settings] [LoadDistrConfig] file {} couldn't be read successfully", file);
			}

		} catch (std::exception&) {
			logger::warn("[Settings] [LoadDistrConfig] file {} couldn't be read successfully due to an error", file);
		}
	}

	// resolve categories

	LOGLE_1("[Settings] [LoadDistrConfig] resolving categories");

	struct Category
	{
		std::string name;
		std::set<RE::FormID> ids;
		std::set<std::string> cats;
	};

	std::unordered_map<std::string, Category*> cats;
	for (auto& [key, value] : categories) {
		Category* cat = new Category;
		cat->name = key;
		cat->ids = value.first;
		cat->cats = value.second;
		cats.insert_or_assign(key, cat);
	}

	std::set<std::string> catnames;
	std::set<std::string> catnamesresolved;
	// initial iteration over map, to sort out which categories need resolving
	for (auto& [key, value] : categories) {
		if (value.second.size() > 0) {
			catnames.insert(key);
		} else {
			//LOGLE1_1("[Settings] [LoadDistrConfig] already resolved {}", key);
			catnamesresolved.insert(key);
		}
	}

	bool resolvedsomething = false;
	int count = 0;


	while (catnames.size() > 0 && count < 1000) {
		count++;
		resolvedsomething = false;
		for (auto& key : catnames) {
			//LOGLE1_1("[Settings] [LoadDistrConfig] resolving {}", key);
			auto itr = cats.find(key);
			if (itr != cats.end()) {
				for (auto& cat : itr->second->cats) {
					//LOGLE2_1("[Settings] [LoadDistrConfig] resolving {} and {}", key, cat);
					if (catnamesresolved.contains(cat)) {
						//LOGLE_1("[Settings] [LoadDistrConfig] 1");
						// category is already resolved, so get the ids
						auto itra = cats.find(cat);
						if (itra != cats.end()) {
							//LOGLE_1("[Settings] [LoadDistrConfig] 2");
							for (auto id : itra->second->ids)
								itr->second->ids.insert(id);
							resolvedsomething = true;
							itr->second->cats.erase(cat);
						} else  // category does not exist in current load order, so remove it
						{
							itr->second->cats.erase(cat);
						}
					} else if (catnames.contains(cat)) {
						// don't do anything its not resolved so far
					} else {
						// category does not exist in load order, so remove it
						itr->second->cats.erase(cat);
						// consider this as resolving something, so we do not end up doing bad stuff
						resolvedsomething = true;
					}
				}
				// check whether all categories have been resolved, if so mark this as resolved
				if (itr->second->cats.size() == 0) {
					LOGLE1_1("[Settings] [LoadDistrConfig] resolved {}", key);
					catnamesresolved.insert(key);
					catnames.erase(key);
				}
			} else {
				// fallback cannot happen
				catnames.erase(key);
			}
		}

		if (resolvedsomething == false && catnames.size() > 0) {
			//LOGLE_1("[Settings] [LoadDistrConfig] circle finding");
			// we have a circle somewhere
			// find which categories have circles, and begin resolving the circle by removing a category from one of them

			// create arrays for static checking
			std::vector<std::string> name;
			auto getindex = [&name](std::string cat) {
				for (int i = 0; i < name.size(); i++)
					if (name[i] == cat)
						return i;
				return 0;
			};

			std::vector<std::vector<int>> assoclist;
			for (auto& str : catnames) {
				name.push_back(str);
			}

			// build graph
			for (int i = 0; i < name.size(); i++) {
				assoclist.push_back({});
				auto itr = cats.find(name[i]);
				if (itr != cats.end()) {
					for (auto& cat : itr->second->cats) {
						assoclist[i].push_back(getindex(cat));
					}
				}
			}

			// calculate pathes, find circles and resolve them
			std::vector<std::vector<int>> pathes;
			std::vector<std::vector<int>> pathesnew;
			// init the pathes of length 1 (i.e. themselves
			for (int i = 0; i < name.size(); i++) {
				pathes.push_back({ i });
			}
			// calculate pathes of higher order
			int count = 1;
			bool brk = false;
			while (!brk && count <= name.size() + 1) {
				// calculate longer pathes
				for (int i = 0; i < pathes.size(); i++) {
					if (assoclist[pathes[i].back()].size() != 0) {
						for (int c = 0; c < assoclist[pathes[i].back()].size(); c++) {
							auto& path = pathes[i];
							std::vector<int> newpath;
							for (auto x : path)
								newpath.push_back(x);
							newpath.push_back((assoclist[pathes[i].back()])[c]);
							pathesnew.push_back(newpath);
						}
					} else {
						// path has ended and cannot be a circle
					}
				}
				// find circles
				std::set<int> nodes;
				for (int i = 0; i < pathesnew.size(); i++) {
					for (int y = 0; y < pathesnew[i].size(); y++) {
						if (nodes.contains(pathesnew[i][y])) {
							// check that we are not at the beginning of the path
							if (y != 0) {
								// found circle, now resolve it
								// since we know the position in the path, we can simply remove the category from the last one in the path so far
								auto iter = cats.find(name[pathesnew[i][y - 1]]);
								if (iter != cats.end()) {
									iter->second->cats.erase(name[pathesnew[i][y]]);

									brk = true;  // break since we found a circle, after we resolve it we have to see whether it resolves itself
								}
							}

						} else
							nodes.insert(pathesnew[i][y]);
					}
					nodes.clear();
				}
				// move new pathes to pathes
				pathes.swap(pathesnew);
				pathesnew.clear();
			}
			count++;
		}
	}
	/*if (count >= 1000)
	{
		exit(2);
	}*/

	LOGLE1_1("[Settings] [LoadDistrConfig] resolving categories took {} iterations", count);

	for (auto& [key, value] : cats)
	{
		LOGLE2_1("[Settings] [LoadDistrConfig] Category, {} : {}", key, UtilityAlch::Concat(value->ids));
	}


	LOGLE_1("[Settings] [LoadDistrConfig] resolving infection rules");

	// resolve infection rules
	for (auto& [disease, chance, scale, cates] : infecrules) {
		LOGLE2_1("[Settings] [LoadDistrConfig] Infec , {} : {}", UtilityAlch::ToString(disease), UtilityAlch::Concat(cates));
		std::set<RE::FormID> ids;
		// get all formIDS for the rule
		for (auto& name : cates) {
			auto itr = cats.find(name);
			if (itr != cats.end()) {
				for (auto id : itr->second->ids)
					ids.insert(id);
			}
		}

		auto data = Data::GetSingleton();
		for (auto id : ids) {
			auto itr = data->diseasesAssoc.find(id);
			if (itr != data->diseasesAssoc.end()) {
				if (itr->second)  // vector valid
				{
					//LOGLE_2("[Settings] [LoadDistrRules] Define probable infections 3");
					itr->second->push_back(std::tuple<Diseases::Disease, float, float>{ disease, chance, scale });
				} else  // vector not valid
				{
					//LOGLE_2("[Settings] [LoadDistrRules] Define probable infections 4");
					std::unique_ptr<std::vector<std::tuple<Diseases::Disease, float, float>>> vec = std::make_unique<std::vector<std::tuple<Diseases::Disease, float, float>>>();
					vec->push_back(std::tuple<Diseases::Disease, float, float>{ disease, chance, scale });
					data->diseasesAssoc.insert_or_assign(id, std::move(vec));
				}
			} else {
				//LOGLE_2("[Settings] [LoadDistrRules] Define probable infections 4");
				std::unique_ptr<std::vector<std::tuple<Diseases::Disease, float, float>>> vec = std::make_unique<std::vector<std::tuple<Diseases::Disease, float, float>>>();
				vec->push_back(std::tuple<Diseases::Disease, float, float>{ disease, chance, scale });
				data->diseasesAssoc.insert_or_assign(id, std::move(vec));
			}
		}
	}

	for (auto& [key, value] : Data::GetSingleton()->diseasesAssoc)
	{
		LOGLE2_1("[Settings] [LoadDistrConfig] Assoc, {} : {}", Utility::GetHex(key), UtilityAlch::Concat(value.get()));
	}

	for (auto& [key, cat] : cats)
	{
		delete cat;
	}
	cats.clear();

	LOGLE_1("[Settings] [LoadDistrConfig] resolved rules");

	LoadGameObjects();


	if (Logging::EnableLog) {
		logger::info("[Settings] [LoadDistrConfig] DiseasesAssoc: {}", Data::GetSingleton()->diseasesAssoc.size());
		logger::info("[Settings] [LoadDistrConfig] DiseasesForceAssoc: {}", Data::GetSingleton()->diseasesForceAssoc.size());
	}
}

void Settings::ApplySkillBoostPerks()
{
	auto races = CalcRacesWithoutPotionSlot();
	auto datahandler = RE::TESDataHandler::GetSingleton();
	RE::BSTArray<RE::TESNPC*> npcs = datahandler->GetFormArray<RE::TESNPC>();
	for (auto& npc : npcs) {
		// make sure it isn't the player, isn't excluded, and the race isn't excluded from the perks
		if (npc && npc->GetFormID() != 0x7 && npc->GetRace() && !Distribution::ExcludedNPC(npc) && races.contains(npc->GetRace()->GetFormID()) == false) {
			// some creatures have cause CTDs or other problems, if they get the perks, so try to filter some of them out
			// if they are a creature and do not have any explicit rule, they will not get any perks
			// at the same time, their id will be blacklisted for the rest of the plugin, to avoid any handling and distribution problems
			if (Settings::NUPSettings::Compatibility::_DisableCreaturesWithoutRules && (npc->GetRace()->HasKeyword(Settings::GameObj::ActorTypeCreature) || npc->GetRace()->HasKeyword(GameObj::ActorTypeAnimal))) {
				ActorStrength acs;
				ItemStrength is;
				auto tplt = Utility::ExtractTemplateInfo(npc);
				auto rule = Distribution::CalcRule(npc, acs, is, &tplt);
				if (rule->ruleName == Distribution::emptyRule->ruleName || rule->ruleName == Distribution::defaultRule->ruleName) {
					// blacklist the npc
					Distribution::_excludedNPCs.insert(npc->GetFormID());
					//logwarn("[Settings] [AddPerks] Excluded creature {}", Utility::PrintForm(npc));
					// handle next npc
					continue;
				}
			}
			npc->AddPerk(Settings::GameObj::AlchemySkillBoosts, 1);
			npc->AddPerk(Settings::GameObj::PerkSkillBoosts, 1);
			//LOGL1_4("{}[Settings] [AddPerks] Added perks to npc {}", Utility::PrintForm(npc));
		}
	}
}

void Settings::ClassifyItems()
{
	// resetting all items
	_itemsInit = false;

	_potionsWeak_main.clear();
	_potionsWeak_rest.clear();
	_potionsStandard_main.clear();
	_potionsStandard_rest.clear();
	_potionsPotent_main.clear();
	_potionsPotent_rest.clear();
	_potionsInsane_main.clear();
	_potionsInsane_rest.clear();
	_potionsBlood.clear();
	_poisonsWeak.clear();
	_poisonsStandard.clear();
	_poisonsPotent.clear();
	_poisonsInsane.clear();
	_foodall.clear();

	Data* data = Data::GetSingleton();
	data->ResetAlchItemEffects();
	Comp* comp = Comp::GetSingleton();

	std::vector<std::tuple<std::string, std::string>> ingredienteffectmap;

	// start sorting items

	auto begin = std::chrono::steady_clock::now();
	const auto& [hashtable, lock] = RE::TESForm::GetAllForms();
	{
		const RE::BSReadLockGuard locker{ lock };
		if (hashtable) {
			RE::AlchemyItem* item = nullptr;
			RE::IngredientItem* itemi = nullptr;
			for (auto& [id, form] : *hashtable) {
				if (form && form->IsMagicItem()) {
					item = form->As<RE::AlchemyItem>();
					if (item) {
						//LOGL1_4("{}[Settings] [ClassifyItems] Found AlchemyItem {}", Utility::PrintForm(item));
						// unnamed items cannot appear in anyones inventory normally so son't add them to our lists
						if (item->GetName() == nullptr || item->GetName() == (const char*)"" || strlen(item->GetName()) == 0 ||
							std::string(item->GetName()).find(std::string("Dummy")) != std::string::npos ||
							std::string(item->GetName()).find(std::string("dummy")) != std::string::npos) {
							continue;
						}
						// check whether item is excluded, or whether it is not whitelisted when in whitelist mode
						// if it is excluded and whitelisted it is still excluded
						if (Distribution::excludedItems()->contains(item->GetFormID()) ||
							Settings::NUPSettings::Whitelist::EnabledItems &&
								!Distribution::whitelistItems()->contains(item->GetFormID())) {
							continue;
						}
						// check whether the plugin is excluded
						if (Distribution::excludedPlugins()->contains(Utility::Mods::GetPluginIndex(item)) == true) {
							continue;
						}

						auto clas = ClassifyItem(item);

						// there is a little bit of a problem for some items that have wrong flags and no keywords set. Try to detect them by sound and set the flags
						if (item->IsFood() == false && item->IsMedicine() == false && item->IsPoison() == false && item->HasKeyword(Settings::GameObj::VendorItemFood) == false && item->HasKeyword(Settings::GameObj::VendorItemFoodRaw) == false && item->HasKeyword(Settings::GameObj::VendorItemPoison) == false && item->HasKeyword(Settings::GameObj::VendorItemPotion) == false) {
							if (item->data.consumptionSound == Settings::GameObj::FoodEat) {
								item->data.flags = RE::AlchemyItem::AlchemyFlag::kFoodItem | item->data.flags;
								//} else if (item->data.consumptionSound == Settings::PoisonUse) {
								//	item->data.flags = RE::AlchemyItem::AlchemyFlag::kPoison | item->data.flags;
							} else if (item->data.consumptionSound == Settings::GameObj::PotionUse) {
								item->data.flags = RE::AlchemyItem::AlchemyFlag::kMedicine | item->data.flags;
							}
						}
						// set medicine flag for those who need it
						if (item->IsFood() == false && item->IsPoison() == false) {  //  && item->IsMedicine() == false
							item->data.flags = RE::AlchemyItem::AlchemyFlag::kMedicine | item->data.flags;
							if (Logging::EnableLoadLog && Logging::LogLevel >= 4) {
								//LOGLE1_1("Item: {}", Utility::PrintForm(item));
								if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kCostOverride)
									LOGLE_1("\tFlag: CostOverride");
								if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kFoodItem)
									LOGLE_1("\tFlag: FoodItem");
								if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kExtendDuration)
									LOGLE_1("\tFlag: ExtendedDuration");
								if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kMedicine)
									LOGLE_1("\tFlag: Medicine");
								if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kPoison)
									LOGLE_1("\tFlag: Poison");
							}
							//LOGLE1_1("[Settings] [ClassifyItems] [AssignPotionFlag] {}", Utility::PrintForm(item));
						}
						// exclude item, if it has an alchemy effect that has been excluded
						AlchemicEffect effects = std::get<0>(clas);
						auto itr = Distribution::excludedEffects()->begin();
						while (itr != Distribution::excludedEffects()->end()) {
							if ((effects & *itr).IsValid()) {
								Distribution::_excludedItems.insert(item->GetFormID());
							}
							itr++;
						}
						if (Distribution::excludedItems()->contains(item->GetFormID())) {
							continue;
						}

						// if the item has the ReflectDamage effect, with a strength of more than 50%, remove the item
						if ((std::get<0>(clas) & AlchemicEffect::kReflectDamage).IsValid()) {
							for (int i = 0; i < (int)item->effects.size(); i++) {
								if (item->effects[i]->baseEffect &&
									((ConvertToAlchemyEffectPrimary(item->effects[i]->baseEffect) == AlchemicEffect::kReflectDamage) ||
										(item->effects[i]->baseEffect->data.archetype == RE::EffectArchetypes::ArchetypeID::kDualValueModifier && (ConvertToAlchemyEffectSecondary(item->effects[i]->baseEffect) == AlchemicEffect::kReflectDamage)))) {
									if (item->effects[i]->effectItem.magnitude > 50) {
										Distribution::_excludedItems.insert(item->GetFormID());
										//LOGLE1_1("[Settings] [ClassifyItems] Excluded {} due to strong ReflectDamage effect", Utility::PrintForm(item));
										continue;
									}
								}
							}
						}

						// check if item has known alcohol keywords and add it to list of alcohol
						if (comp->LoadedCACO() && item->HasKeyword(comp->CACO_VendorItemDrinkAlcohol) || comp->LoadedApothecary() && item->HasKeyword(comp->Apot_SH_AlcoholDrinkKeyword)) {
							Distribution::_alcohol.insert(item->GetFormID());
						}

						// check for player excluded magiceffects
						for (int i = 0; i < (int)item->effects.size(); i++) {
							if (item->effects[i]->baseEffect && Distribution::excludedItemsPlayer()->contains(item->effects[i]->baseEffect->GetFormID())) {
								//LOGLE1_1("[Settings] [ClassifyItems] Excluded {} for player due to effect", Utility::PrintForm(item));
								Distribution::_excludedItemsPlayer.insert(item->GetFormID());
							}
						}

						// since the item is not to be excluded, save which alchemic effects are present
						_alchemyEffectsFound |= std::get<0>(clas);

						RE::Actor* player = RE::PlayerCharacter::GetSingleton();

						// if the value of the item is less than zero, we should not insert them into the distribution lists, since they are likely to be broken
						// or test/dummy items
						if (item->CalculateTotalGoldValue(player)) {
							// determine the type of item
							if (std::get<2>(clas) == ItemType::kFood &&
								(Settings::NUPSettings::Food::_AllowDetrimentalEffects || std::get<5>(clas) == false /*either we allow detrimental effects or there are none*/)) {
								_foodall.insert(_foodall.end(), { std::get<0>(clas), item });
								_foodEffectsFound |= std::get<0>(clas);
							} else if (std::get<2>(clas) == ItemType::kPoison &&
									   (Settings::NUPSettings::Poisons::_AllowPositiveEffects || std::get<5>(clas) == false /*either we allow positive effects or there are none*/)) {
								switch (std::get<1>(clas)) {
								case ItemStrength::kWeak:
									_poisonsWeak.insert(_poisonsWeak.end(), { std::get<0>(clas), item });
									break;
								case ItemStrength::kStandard:
									_poisonsStandard.insert(_poisonsStandard.end(), { std::get<0>(clas), item });
									break;
								case ItemStrength::kPotent:
									_poisonsPotent.insert(_poisonsPotent.end(), { std::get<0>(clas), item });
									break;
								case ItemStrength::kInsane:
									_poisonsInsane.insert(_poisonsInsane.end(), { std::get<0>(clas), item });
									break;
								}
								_poisonEffectsFound |= std::get<0>(clas);
							} else if (std::get<2>(clas) == ItemType::kPotion &&
									   (Settings::NUPSettings::Potions::_AllowDetrimentalEffects || std::get<5>(clas) == false /*either we allow detrimental effects or there are none*/)) {
								if ((std::get<0>(clas) & AlchemicEffect::kBlood) > 0)
									_potionsBlood.insert(_potionsBlood.end(), { std::get<0>(clas), item });
								else if ((std::get<0>(clas) & AlchemicEffect::kHealth) > 0 ||
										 (std::get<0>(clas) & AlchemicEffect::kMagicka) > 0 ||
										 (std::get<0>(clas) & AlchemicEffect::kStamina) > 0) {
									switch (std::get<1>(clas)) {
									case ItemStrength::kWeak:
										_potionsWeak_main.insert(_potionsWeak_main.end(), { std::get<0>(clas), item });
										break;
									case ItemStrength::kStandard:
										_potionsStandard_main.insert(_potionsStandard_main.end(), { std::get<0>(clas), item });
										break;
									case ItemStrength::kPotent:
										_potionsPotent_main.insert(_potionsPotent_main.end(), { std::get<0>(clas), item });
										break;
									case ItemStrength::kInsane:
										_potionsInsane_main.insert(_potionsPotent_main.end(), { std::get<0>(clas), item });
										break;
									}
								} else if (std::get<0>(clas) != AlchemicEffect::kNone) {
									switch (std::get<1>(clas)) {
									case ItemStrength::kWeak:
										_potionsWeak_rest.insert(_potionsWeak_rest.end(), { std::get<0>(clas), item });
										break;
									case ItemStrength::kStandard:
										_potionsStandard_rest.insert(_potionsStandard_rest.end(), { std::get<0>(clas), item });
										break;
									case ItemStrength::kPotent:
										_potionsPotent_rest.insert(_potionsPotent_rest.end(), { std::get<0>(clas), item });
										break;
									case ItemStrength::kInsane:
										_potionsInsane_rest.insert(_potionsInsane_rest.end(), { std::get<0>(clas), item });
										break;
									}
								}
								_potionEffectsFound |= std::get<0>(clas);
							}
						} else {
							//LOGLE1_1("[Settings] [ClassifyItems] Item {} has value 0 and will not be distributed", Utility::PrintForm(item));
						}
						int dosage = 0;
						if (item->IsPoison())
							dosage = Distribution::GetPoisonDosage(item, std::get<0>(clas));
						// add item into effect map
						data->SetAlchItemEffects(item->GetFormID(), std::get<0>(clas), std::get<3>(clas), std::get<4>(clas), std::get<5>(clas), dosage);
						//LOGLE4_1("[Settings] [ClassifyItems] Saved effects for {} dur {} mag {} effect {}", Utility::PrintForm(item), std::get<3>(clas), std::get<4>(clas), std::get<0>(clas).string());
					}

					itemi = form->As<RE::IngredientItem>();
					if (itemi) {
						//LOGL1_4("{}[Settings] [ClassifyItems] Found IngredientItem {}", Utility::PrintForm(itemi));
						for (int i = 0; i < (int)itemi->effects.size(); i++) {
							auto sett = itemi->effects[i]->baseEffect;
							// just retrieve the effects, we will analyze them later
							if (sett) {
								ingredienteffectmap.push_back({ itemi->GetName(), Utility::ToString(ConvertToAlchemyEffectPrimary(sett)) });

								// the effects of ingredients may lead to valid potions being brewed, so we need to save that these effects actually exist in the game
								_alchemyEffectsFound |= ConvertToAlchemyEffectPrimary(sett);
							}
						}
					}
				}
			}
		}
	}
	PROF1_1("{}[ClassifyItems] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));

	// add alcoholic items to player exclusion list
	if (Settings::NUPSettings::Player::_DontDrinkAlcohol) {
		auto itr = Distribution::_alcohol.begin();
		while (itr != Distribution::_alcohol.end()) {
			Distribution::_excludedItemsPlayer.insert(*itr);
			itr++;
		}
	}

	// items initialised
	_itemsInit = true;
	/*
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsWeak_main {}", potionsWeak_main()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsWeak_rest {}", potionsWeak_rest()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsStandard_main {}", potionsStandard_main()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsStandard_rest {}", potionsStandard_rest()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsPotent_main {}", potionsPotent_main()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsPotent_rest {}", potionsPotent_rest()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsInsane_main {}", potionsInsane_main()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsInsane_rest {}", potionsInsane_rest()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsBlood {}", potionsBlood()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _poisonsWeak {}", poisonsWeak()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _poisonsStandard {}", poisonsStandard()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _poisonsPotent {}", poisonsPotent()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _poisonsInsane {}", poisonsInsane()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _foodall {}", foodall()->size());

	if (Logging::EnableLoadLog && Logging::LogLevel >= 4) {
		std::string path = "Data\\SKSE\\Plugins\\NPCsUsePotions\\items.txt";
		std::ofstream out = std::ofstream(path, std::ofstream::out);
		std::unordered_set<RE::FormID> visited;
		out << "potionsWeak_main\n";
		auto it = potionsWeak_main()->begin();
		while (it != potionsWeak_main()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "potionsStandard_main\n";
		it = potionsStandard_main()->begin();
		while (it != potionsStandard_main()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "potionsStandard_rest\n";
		it = potionsStandard_rest()->begin();
		while (it != potionsStandard_rest()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "potionsPotent_main\n";
		it = potionsPotent_main()->begin();
		while (it != potionsPotent_main()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "potionsInsane_main\n";
		it = potionsInsane_main()->begin();
		while (it != potionsInsane_main()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "potionsBlood\n";
		it = potionsBlood()->begin();
		while (it != potionsBlood()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "poisonsWeak\n";
		it = poisonsWeak()->begin();
		while (it != poisonsWeak()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "poisonsStandard\n";
		it = poisonsStandard()->begin();
		while (it != poisonsStandard()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "poisonsPotent\n";
		it = poisonsPotent()->begin();
		while (it != poisonsPotent()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "poisonsInsane\n";
		it = poisonsInsane()->begin();
		while (it != poisonsInsane()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}
		out << "foodall\n";
		it = foodall()->begin();
		while (it != foodall()->end()) {
			if (!visited.contains(std::get<1>(*it)->GetFormID()))
				out << ";" << std::get<1>(*it)->GetName() << "\n"
					<< "1|7|<" << Utility::GetHex(std::get<1>(*it)->GetFormID()) << ",>\n";
			visited.insert(std::get<1>(*it)->GetFormID());
			it++;
		}

		std::string pathing = "Data\\SKSE\\Plugins\\NPCsUsePotions\\ingredients.csv";
		std::ofstream outing = std::ofstream(pathing, std::ofstream::out);
		for (int i = 0; i < ingredienteffectmap.size(); i++) {
			outing << std::get<0>(ingredienteffectmap[i]) << ";" << std::get<1>(ingredienteffectmap[i]) << "\n";
		}
	}*/
}

std::tuple<AlchemicEffect, ItemStrength, ItemType, int, float, bool> Settings::ClassifyItem(RE::AlchemyItem* item)
{
	RE::EffectSetting* sett = nullptr;
	if ((item->avEffectSetting) == nullptr && item->effects.size() == 0) {
		return { 0, ItemStrength::kStandard, ItemType::kFood, 0, 0.0f, false };
	}
	// we look at max 4 effects
	AlchemicEffect av[4]{
		0,
		0,
		0,
		0
	};
	float mag[]{
		0,
		0,
		0,
		0
	};
	int dur[]{
		0,
		0,
		0,
		0
	};
	bool detrimental = false;
	bool positive = false;
	// we will not abort the loop, since the number of effects on one item is normally very
	// limited, so we don't have much iterations
	AlchemicEffect tmp = 0;
	if (item->effects.size() > 0) {
		for (uint32_t i = 0; i < item->effects.size() && i < 4; i++) {
			sett = item->effects[i]->baseEffect;
			// just retrieve the effects, we will analyze them later
			if (sett) {
				mag[i] = item->effects[i]->effectItem.magnitude;
				dur[i] = item->effects[i]->effectItem.duration;

				// force area to zero, to avoid CTDs when using the item.
				item->effects[i]->effectItem.area = 0;

				detrimental |= sett->IsDetrimental();
				positive |= !sett->IsDetrimental();

				uint32_t formid = sett->GetFormID();
				if ((tmp = (ConvertToAlchemyEffectPrimary(sett))) > 0) {
					av[i] |= tmp;
				}
				if (sett->data.archetype == RE::EffectArchetypes::ArchetypeID::kDualValueModifier && (tmp = ConvertToAlchemyEffectSecondary(sett)) > 0) {
					av[i] |= tmp;
				}
				// we only need this for magnitude calculations, so its not used as cooldown
				if (dur[i] == 0)
					dur[i] = 1;
			}
		}
	} else {
		// emergency fallback // more or less unused
		RE::MagicItem::SkillUsageData err;
		item->GetSkillUsageData(err);
		detrimental |= item->avEffectSetting->IsDetrimental();
		positive |= !item->avEffectSetting->IsDetrimental();
		switch (item->avEffectSetting->data.primaryAV) {
		case RE::ActorValue::kHealth:
			av[0] = ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV);
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		case RE::ActorValue::kMagicka:
			av[0] = ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV);
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		case RE::ActorValue::kStamina:
			av[0] = ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV);
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		}
	}
	// analyze the effect types
	AlchemicEffect alch = 0;
	ItemStrength str = ItemStrength::kWeak;
	float maxmag = 0;
	int maxdur = 0;
	for (int i = 0; i < 4; i++) {
		if (mag[i] == 0)
			mag[i] = 1;
		if (dur[i] == 0)
			dur[i] = 1;
		if (mag[i] * dur[i] > maxmag) {
			maxmag = mag[i] * dur[i];
			maxdur = dur[i];
		}
		alch |= av[i];
	}
	if (std::string(item->GetName()).find(std::string("Weak")) != std::string::npos)
		str = ItemStrength::kWeak;
	else if (std::string(item->GetName()).find(std::string("Standard")) != std::string::npos)
		str = ItemStrength::kStandard;
	else if (std::string(item->GetName()).find(std::string("Potent")) != std::string::npos)
		str = ItemStrength::kPotent;
	else if (maxmag == 0)
		str = ItemStrength::kStandard;
	else if (maxmag <= NUPSettings::Distr::_MaxMagnitudeWeak)
		str = ItemStrength::kWeak;
	else if (maxmag <= NUPSettings::Distr::_MaxMagnitudeStandard)
		str = ItemStrength::kStandard;
	else if (maxmag <= NUPSettings::Distr::_MaxMagnitudePotent)
		str = ItemStrength::kPotent;
	else
		str = ItemStrength::kInsane;
	auto iter = Distribution::itemStrengthMap()->find(item->GetFormID());
	if (iter != Distribution::itemStrengthMap()->end()) {
		str = iter->second;
	}

	// if the potion is a blood potion it should only ever appear on vampires, no the
	// effects are overriden to AlchemyEffect::kBlood
	if (std::string(item->GetName()).find(std::string("Blood")) != std::string::npos &&
		std::string(item->GetName()).find(std::string("Potion")) != std::string::npos) {
		alch = AlchemicEffect::kBlood;
		// if we have a blood potion, make sure that it has the medicine flag
		if (item->IsMedicine() == false)
			item->data.flags = RE::AlchemyItem::AlchemyFlag::kMedicine | item->data.flags;
	}

	ItemType type = ItemType::kPotion;
	if (item->IsFood() || item->HasKeyword(GameObj::VendorItemFood) || item->HasKeyword(GameObj::VendorItemFoodRaw))
		type = ItemType::kFood;
	else if (item->IsPoison() || item->HasKeyword(Settings::GameObj::VendorItemPoison)) {
		type = ItemType::kPoison;
		return {
			alch,
			str,
			type,
			maxdur,
			maxmag,
			positive  // return whether there is a positive effect on the poison
		};
	}

	return {
		alch,
		str,
		type,
		maxdur,
		maxmag,
		detrimental
	};
}

#pragma endregion
