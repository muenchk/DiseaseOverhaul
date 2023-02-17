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

#include "Settings.h"
#include "Data.h"
#include "UtilityAlch.h"

using ActorStrength = ActorStrength;
using AlchemyEffect = AlchemyEffect;
using ItemStrength = ItemStrength;
using ItemType = Settings::ItemType;

static std::mt19937 randi((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
/// <summary>
/// trims random numbers to 1 to RR
/// </summary>
static std::uniform_int_distribution<signed> randRR(1, RandomRange);
static std::uniform_int_distribution<signed> rand100(1, 100);

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

	/// load disease objects
}

void Settings::LoadDistrConfigNUP()
{
	LOG_1("{}[Settings] [LoadDistrConfigNUP]");
	// set to false, to avoid other funcions running stuff on our variables
	Distribution::initialised = false;

	std::vector<std::string> files;
	auto constexpr folder = R"(Data\SKSE\Plugins\)";
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini") {
			if (auto path = entry.path().string(); path.rfind("NUP_DIST") != std::string::npos) {
				files.push_back(path);
				logger::info("[Settings] [LoadDistrRules] found Distribution configuration file: {}", entry.path().filename().string());
			}
		}
	}
	if (files.empty()) {
		logger::info("[Settings] [LoadDistrRules] No Distribution files were found");
	}
	// init datahandler
	auto datahandler = RE::TESDataHandler::GetSingleton();

	// vector of splits, filename and line
	std::vector<std::tuple<std::vector<std::string>*, std::string, std::string>> attachments;
	std::vector<std::tuple<std::vector<std::string>*, std::string, std::string>> copyrules;

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
						logger::warn("[Settings] [LoadDistrRules] Not a rule. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what rule version we have
					int ruleVersion = -1;
					try {
						ruleVersion = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what kind of rule we have
					int ruleType = -1;
					try {
						ruleType = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
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
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 25. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										delete splits;
										continue;
									}
									// next entry is the rulename, so we just set it
									Distribution::Rule* rule = new Distribution::Rule();
									rule->ruleVersion = ruleVersion;
									rule->ruleType = ruleType;
									rule->ruleName = splits->at(splitindex);
									LOGE1_2("[Settings] [LoadDistrRules] loading rule: {}", rule->ruleName);
									splitindex++;
									// now come the rule priority
									rule->rulePriority = -1;
									try {
										rule->rulePriority = std::stoi(splits->at(splitindex));
										splitindex++;
									} catch (std::out_of_range&) {
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"RulePrio\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RulePrio\". file: {}, rule:\"{}\"", file, tmp);
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
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"MaxPotions\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"MaxPotions\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion1Chance
									rule->potion1Chance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion1Chance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"Potion1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion2Chance
									rule->potion2Chance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion2Chance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"Potion2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Potion3Chance
									rule->potion3Chance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potion3Chance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"Potion3Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PotionAddChance
									rule->potionAdditionalChance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->potionAdditionalChance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"PotionAddChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
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
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"PotionsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"PotionsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Fortify1Chance
									rule->fortify1Chance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortify1Chance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"Fortify1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Fortify2Chance
									rule->fortify2Chance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->fortify2Chance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"Fortify2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
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
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"MaxPoisons\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"MaxPoisons\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison1Chance
									rule->poison1Chance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison1Chance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"Poison1Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison2Chance
									rule->poison2Chance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison2Chance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"Poison2Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes Poison3Chance
									rule->poison3Chance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poison3Chance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"Poison3Chance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}
									// now comes PoisonAddChance
									rule->poisonAdditionalChance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->poisonAdditionalChance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"PoisonAddChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
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
										logger::warn("[Settings] [LoadDistrRules] out-of-range expection in field \"PoisonsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									} catch (std::invalid_argument&) {
										logger::warn("[Settings] [LoadDistrRules] invalid-argument expection in field \"PoisonsTierAdjust\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										delete rule;
										continue;
									}

									// now comes FoodChance
									rule->foodChance = UtilityAlch::ParseIntArray(splits->at(splitindex));
									splitindex++;
									if (rule->foodChance.size() == 0) {
										logger::warn("[Settings] [LoadDistrRules] fiels \"FoodChance\" couldn't be parsed. file: {}, rule:\"{}\"", file, tmp);
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
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> objects = UtilityAlch::ParseAssocObjects(rule->assocObjects, error, file, line);

									// parse the item properties
									std::vector<std::tuple<uint64_t, float>> potioneffects = UtilityAlch::ParseAlchemyEffects(rule->potionProperties, error);
									rule->potionDistr = UtilityAlch::GetDistribution(potioneffects, RandomRange);
									rule->potionDistrChance = UtilityAlch::GetDistribution(potioneffects, RandomRange, true);
									LOGE2_2("[Settings] [LoadDistrRules] rule {} contains {} potion effects", rule->ruleName, rule->potionDistr.size());
									rule->validPotions = UtilityAlch::SumAlchemyEffects(rule->potionDistr, true);
									std::vector<std::tuple<uint64_t, float>> poisoneffects = UtilityAlch::ParseAlchemyEffects(rule->poisonProperties, error);
									rule->poisonDistr = UtilityAlch::GetDistribution(poisoneffects, RandomRange);
									rule->poisonDistrChance = UtilityAlch::GetDistribution(poisoneffects, RandomRange, true);
									LOGE2_2("[Settings] [LoadDistrRules] rule {} contains {} poison effects", rule->ruleName, rule->poisonDistr.size());
									rule->validPoisons = UtilityAlch::SumAlchemyEffects(rule->poisonDistr, true);
									std::vector<std::tuple<uint64_t, float>> fortifyeffects = UtilityAlch::ParseAlchemyEffects(rule->fortifyproperties, error);
									rule->fortifyDistr = UtilityAlch::GetDistribution(fortifyeffects, RandomRange);
									rule->fortifyDistrChance = UtilityAlch::GetDistribution(fortifyeffects, RandomRange, true);
									LOGE2_2("[Settings] [LoadDistrRules] rule {} contains {} fortify potion effects", rule->ruleName, rule->fortifyDistr.size());
									rule->validFortifyPotions = UtilityAlch::SumAlchemyEffects(rule->fortifyDistr, true);
									std::vector<std::tuple<uint64_t, float>> foodeffects = UtilityAlch::ParseAlchemyEffects(rule->foodProperties, error);
									rule->foodDistr = UtilityAlch::GetDistribution(foodeffects, RandomRange);
									rule->foodDistrChance = UtilityAlch::GetDistribution(foodeffects, RandomRange, true);
									LOGE2_2("[Settings] [LoadDistrRules] rule {} contains {} food effects", rule->ruleName, rule->foodDistr.size());
									rule->validFood = UtilityAlch::SumAlchemyEffects(rule->foodDistr, true);

									std::pair<int, Distribution::Rule*> tmptuple = { rule->rulePriority, rule };

									// assign rules to search parameters
									LOGE2_2("[Settings] [LoadDistrRules] rule {} contains {} associated objects", rule->ruleName, objects.size());
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
									LOGE1_2("[Settings] [LoadDistrRules] rule {} successfully loaded.", rule->ruleName);
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
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
									for (int i = 0; i < items.size(); i++) {
										if (std::get<0>(items[i]) == Distribution::AssocType::kActor ||
											std::get<0>(items[i]) == Distribution::AssocType::kNPC ||
											std::get<0>(items[i]) == Distribution::AssocType::kFaction ||
											std::get<0>(items[i]) == Distribution::AssocType::kKeyword ||
											std::get<0>(items[i]) == Distribution::AssocType::kRace) {
											Distribution::_bosses.insert(std::get<1>(items[i]));
											LOGE1_2("[Settings] [LoadDistrRules] declared {} as boss.", UtilityAlch::GetHex(std::get<1>(items[i])));
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 4:  // exclude object
								{
									if (splits->size() != 3) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
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
										if (AlchExtSettings::EnableLog) {
											if (std::get<0>(items[i]) == Distribution::AssocType::kActor ||
												std::get<0>(items[i]) == Distribution::AssocType::kNPC) {
												LOGE1_2("[Settings] [LoadDistrRules] excluded actor {} from distribution.", UtilityAlch::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) == Distribution::AssocType::kFaction) {
												LOGE1_2("[Settings] [LoadDistrRules] excluded faction {} from distribution.", UtilityAlch::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) == Distribution::AssocType::kKeyword) {
												LOGE1_2("[Settings] [LoadDistrRules] excluded keyword {} from distribution.", UtilityAlch::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) == Distribution::AssocType::kItem) {
												LOGE1_2("[Settings] [LoadDistrRules] excluded item {} from distribution.", UtilityAlch::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) == Distribution::AssocType::kRace) {
												LOGE1_2("[Settings] [LoadDistrRules] excluded race {} from distribution.", UtilityAlch::GetHex(std::get<1>(items[i])));
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
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kFaction:
										case Distribution::AssocType::kKeyword:
										case Distribution::AssocType::kRace:
											Distribution::_baselineExclusions.insert(std::get<1>(items[i]));
											break;
										}

										if (AlchExtSettings::EnableLog) {
											if (std::get<0>(items[i]) == Distribution::AssocType::kFaction) {
												LOGE1_2("[Settings] [LoadDistrRules] excluded faction {} from base line distribution.", UtilityAlch::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) == Distribution::AssocType::kKeyword) {
												LOGE1_2("[Settings] [LoadDistrRules] excluded keyword {} from base line distribution.", UtilityAlch::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) == Distribution::AssocType::kRace) {
												LOGE1_2("[Settings] [LoadDistrRules] excluded race {} from base line distribution.", UtilityAlch::GetHex(std::get<1>(items[i])));
											}
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 6: // copy rule
								{
									copyrules.push_back({ splits, file, tmp });
								}
								break;
							case 7: // whitelist rule
								{
									if (splits->size() != 3) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 3. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kItem:
											Distribution::_whitelistItems.insert(std::get<1>(items[i]));
											break;
										}
										if (AlchExtSettings::EnableLog) {
											if (std::get<0>(items[i]) == Distribution::AssocType::kItem) {
												LOGE1_2("[Settings] [LoadDistrRules] whitelisted item {}.", UtilityAlch::GetHex(std::get<1>(items[i])));
											} else if (std::get<0>(items[i]) == Distribution::AssocType::kRace) {
											}
										}
									}
									// since we are done delete splits
									delete splits;
								}
								break;
							case 8: // custom object distribution
								{
									if (splits->size() != 4) {
										logger::warn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 4. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}

									Distribution::CustomItemStorage* citems = new Distribution::CustomItemStorage();
									// parse associated obj
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> assocobj = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
									
									// parse items associated
									assoc = splits->at(splitindex);
									splitindex++;
									error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID, Chance, CustomItemFlag, Number, bool, uint64_t, uint64_t, bool>> associtm = UtilityAlch::ParseCustomObjects(assoc, error, file, tmp);
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
												}
												if (tmpb) {
													if (std::get<5>(associtm[i]))
														Distribution::_excludedItems.insert(std::get<2>(associtm[i]));
													switch (std::get<3>(associtm[i])) {
													case CustomItemFlag::Object:
														{
															citems->items.push_back({ tmpb, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]), std::get<8>(associtm[i]) });
														}
														break;
													case CustomItemFlag::DeathObject:
														{
															citems->death.push_back({ tmpb, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]), std::get<8>(associtm[i]) });
														}
														break;
													default:
														{
															alch = tmpf->As<RE::AlchemyItem>();
															if (alch) {
																switch (std::get<3>(associtm[i])) {
																case CustomItemFlag::Food:
																	citems->food.push_back({ alch, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]) });
																	break;
																case CustomItemFlag::Fortify:
																	citems->fortify.push_back({ alch, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]) });
																	break;
																case CustomItemFlag::Poison:
																	citems->poisons.push_back({ alch, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]) });
																	break;
																case CustomItemFlag::Potion:
																	citems->potions.push_back({ alch, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]) });
																	break;
																}
															} else {
																// item is not an alchemy item so do not add it
																LOGE1_2("[Settings] [LoadDistrRules] custom rule for item {} cannot be applied, due to the item not being an AlchemyItem.", UtilityAlch::GetHex(std::get<1>(associtm[i])));
																//citems->items.push_back({ tmpb, std::get<2>(associtm[i]) });
															}
														}
														break;
													}
												} else {
													LOGE1_2("[Settings] [LoadDistrRules] custom rule for item {} cannot be applied, due to the item not being an TESBoundObject.", UtilityAlch::GetHex(std::get<1>(associtm[i])));
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
										if (AlchExtSettings::EnableLog) {
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
							default:
								logger::warn("[Settings] [LoadDistrRules] Rule type does not exist. file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								break;
							}
						}
						break;
					default:
						logger::warn("[Settings] [LoadDistrRules] Rule version does not exist. file: {}, rule:\"{}\"", file, tmp);
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
		Distribution::defaultRule = new Distribution::Rule(1 /*version*/, 1 /*type*/, DefaultRuleName, INT_MIN + 1 /*rulePriority*/, true /*allowMixed*/, 5 /*maxPotions*/, std::vector<int>{ 30, 40, 50, 60, 70 } /*potion1Chance*/,
			std::vector<int>{ 20, 30, 40, 50, 60 } /*potion2Chance*/, std::vector<int>{ 10, 20, 30, 40, 50 } /*potion3Chance*/, std::vector<int>{ 0, 10, 20, 30, 40 } /*potionAddChance*/,
			std::vector<int>{ 30, 40, 50, 60, 70 } /*fortify1Chance*/, std::vector<int>{ 30, 40, 50, 60, 70 } /*fortify2Chance*/, 0 /*potionTierAdjust*/, 5 /*maxPoisons*/,
			std::vector<int>{ 30, 35, 40, 45, 50 } /*poison1Chance*/, std::vector<int>{ 20, 25, 30, 35, 40 } /*poison2Chance*/, std::vector<int>{ 10, 15, 20, 25, 30 } /*poison3Chance*/,
			std::vector<int>{ 0, 5, 10, 15, 20 } /*poisonAddChance*/, 0 /*poisonTierAdjust*/, std::vector<int>{ 70, 80, 90, 100, 100 } /*foodChance*/,
			Distribution::GetVector(RandomRange, AlchemyEffect::kAnyPotion) /*potionDistr*/,
			Distribution::GetVector(RandomRange, AlchemyEffect::kAnyPoison) /*poisonDistr*/,
			Distribution::GetVector(RandomRange, AlchemyEffect::kAnyFortify) /*fortifyDistr*/,
			Distribution::GetVector(RandomRange, AlchemyEffect::kAnyFood) /*foodDistr*/,
			static_cast<uint64_t>(AlchemyEffect::kAnyPotion) /*validPotions*/,
			static_cast<uint64_t>(AlchemyEffect::kAnyPoison) /*validPoisons*/,
			static_cast<uint64_t>(AlchemyEffect::kAnyFortify) /*validFortifyPotions*/,
			static_cast<uint64_t>(AlchemyEffect::kAnyFood) /*validFood*/);
	}

	if (copyrules.size() > 0) {
		for (auto cpy : copyrules) {
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
			LOGE1_2("[Settings] [LoadDistrRules] rule {} successfully coinialised.", newrule->ruleName);
		}
	}

	// and now for the attachement rules.
	// 
	// vector of splits, filename and line
	//std::vector<std::tuple<std::vector<std::string>*, std::string, std::string>> attachments;
	if (attachments.size() > 0) {
		std::string name;
		for (auto a : attachments) {
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
					std::vector<std::tuple<Distribution::AssocType, RE::FormID>> objects = UtilityAlch::ParseAssocObjects((std::get<0>(a)->at(3)), error, std::get<1>(a), std::get<2>(a));

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
						if (AlchExtSettings::EnableLog) {
							switch (std::get<0>(objects[i])) {
							case Distribution::AssocType::kFaction:
								if (attach) {
									LOGE3_2("[Settings] [LoadDistrRules] attached Faction {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else 
									LOGE5_2("[Settings] [LoadDistrRules] updated Faction {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kKeyword:
								if (attach) {
									LOGE3_2("[Settings] [LoadDistrRules] attached Keyword {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									LOGE5_2("[Settings] [LoadDistrRules] updated Keyword {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kRace:
								if (attach) {
									LOGE3_2("[Settings] [LoadDistrRules] attached Race {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									LOGE5_2("[Settings] [LoadDistrRules] updated Race {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kClass:
								if (attach) {
									LOGE3_2("[Settings] [LoadDistrRules] attached Class {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									LOGE5_2("[Settings] [LoadDistrRules] updated Class {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kCombatStyle:
								if (attach) {
									LOGE3_2("[Settings] [LoadDistrRules] attached CombatStyle {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
								} else
									LOGE5_2("[Settings] [LoadDistrRules] updated CombatStyle {} to rule {} with new Priority {} overruling {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, prio, oldprio, std::get<1>(a));
								break;
							case Distribution::AssocType::kNPC:
							case Distribution::AssocType::kActor:
								if (attach) {
									LOGE3_2("[Settings] [LoadDistrRules] attached Actor {} to rule {}.\t\t\t{}", UtilityAlch::GetHex(std::get<1>(objects[i])), rule->ruleName, std::get<1>(a));
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

	if (Settings::AlchExtSettings::EnableLog) {
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

void Settings::LoadDistrConfigAlchExt()
{
	LOG_1("{}[Settings] [LoadDistrConfigAlchExt]");
	// set to false, to avoid other funcions running stuff on our variables
	Distribution::initialised = false;

	std::vector<std::string> files;
	auto constexpr folder = R"(Data\SKSE\Plugins\)";
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini") {
			if (auto path = entry.path().string(); path.rfind("AlchExt_DIST") != std::string::npos) {
				files.push_back(path);
				logger::info("[SETTINGS] [LoadDistrRulesAlchExt] found Distribution configuration file: {}", entry.path().filename().string());
			}
		}
	}
	if (files.empty()) {
		logger::info("[SETTINGS] [LoadDistrRulesAlchExt] No Distribution files were found");
	}
	// init datahandler
	auto datahandler = RE::TESDataHandler::GetSingleton();

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
					if (line.length() != 0)
						splits->push_back(line);
					int splitindex = 0;
					// check wether we actually have a rule
					if (splits->size() < 3) {  // why 3? Cause first two fields are RuleVersion and RuleType and we don't accept empty rules.
						logger::warn("[Settings] [LoadDistrRulesAlchExt] Not a rule. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what rule version we have
					int ruleVersion = -1;
					try {
						ruleVersion = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						logger::warn("[Settings] [LoadDistrRulesAlchExt] out-of-range expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						logger::warn("[Settings] [LoadDistrRulesAlchExt] invalid-argument expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what kind of rule we have
					int ruleType = -1;
					try {
						ruleType = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						logger::warn("[Settings] [LoadDistrRulesAlchExt] out-of-range expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						logger::warn("[Settings] [LoadDistrRulesAlchExt] invalid-argument expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
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
										logger::warn("[Settings] [LoadDistrRulesAlchExt] rule has wrong number of fields, expected 4. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}

									Distribution::CustomItemStorage* citems = new Distribution::CustomItemStorage();
									// parse associated obj
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> assocobj = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);

									// parse items associated
									assoc = splits->at(splitindex);
									splitindex++;
									error = false;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID, Chance, CustomItemFlag, Number, bool, uint64_t, uint64_t, bool>> associtm = UtilityAlch::ParseCustomObjects(assoc, error, file, tmp);
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
												}
												if (tmpb) {
													if (std::get<5>(associtm[i]))
														Distribution::_excludedItems.insert(std::get<2>(associtm[i]));
													switch (std::get<3>(associtm[i])) {
													case CustomItemFlag::Object:
														{
															citems->items.push_back({ tmpb, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]), std::get<8>(associtm[i]) });
														}
														break;
													case CustomItemFlag::DeathObject:
														{
															citems->death.push_back({ tmpb, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]), std::get<8>(associtm[i]) });
														}
														break;
													default:
														{
															alch = tmpf->As<RE::AlchemyItem>();
															if (alch) {
																switch (std::get<3>(associtm[i])) {
																case CustomItemFlag::Food:
																	citems->food.push_back({ alch, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]) });
																	break;
																case CustomItemFlag::Fortify:
																	citems->fortify.push_back({ alch, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]) });
																	break;
																case CustomItemFlag::Poison:
																	citems->poisons.push_back({ alch, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]) });
																	break;
																case CustomItemFlag::Potion:
																	citems->potions.push_back({ alch, std::get<2>(associtm[i]), std::get<4>(associtm[i]), std::get<6>(associtm[i]), std::get<7>(associtm[i]) });
																	break;
																}
															} else {
																// item is not an alchemy item so do not add it
																LOGE1_2("[Settings] [LoadDistrRules] custom rule for item {} cannot be applied, due to the item not being an AlchemyItem.", UtilityAlch::GetHex(std::get<1>(associtm[i])));
																//citems->items.push_back({ tmpb, std::get<2>(associtm[i]) });
															}
														}
														break;
													}
												} else {
													LOGE1_2("[Settings] [LoadDistrRules] custom rule for item {} cannot be applied, due to the item not being an TESBoundObject.", UtilityAlch::GetHex(std::get<1>(associtm[i])));
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
										if (AlchExtSettings::EnableLog) {
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
							case 101: // disease: define infected
								{
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
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
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
												auto itr = data->diseasesAssoc.find(std::get<1>(items[i]));
												if (itr != data->diseasesAssoc.end()) {
													if (itr->second)  // vector valid
													{
														itr->second->push_back(disease);
													} else  // vector not valid
													{
														std::vector<Diseases::Disease>* vec = new std::vector<Diseases::Disease>;
														vec->push_back(disease);
														data->diseasesAssoc.insert_or_assign(std::get<1>(items[i]), vec);
													}
												}
											}
											break;
										}
									}
									delete splits;
								}
								break;
							case 102: // disease: define cell properties
								{
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
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
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
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
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
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = UtilityAlch::ParseAssocObjects(assoc, error, file, tmp);
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
							default:
								logger::warn("[Settings] [LoadDistrRulesAlchExt] Rule type does not exist. file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								break;
							}
						}
						break;
					default:
						logger::warn("[Settings] [LoadDistrRulesAlchExt] Rule version does not exist. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						break;
					}
				}
			} else {
				logger::warn("[Settings] [LoadDistrRulesAlchExt] file {} couldn't be read successfully", file);
			}

		} catch (std::exception&) {
			logger::warn("[Settings] [LoadDistrRulesAlchExt] file {} couldn't be read successfully due to an error", file);
		}
	}

	LoadGameObjects();


	if (Settings::AlchExtSettings::EnableLog) {
		//logger::info("[Settings] [LoadDistrRulesAlchExt] Number of Rules: {}", Distribution::rules()->size());
	}
}

static bool IsLeveledChar(RE::TESNPC* npc)
{
	if (npc->baseTemplateForm == nullptr)
		return true;
	RE::TESNPC* tplt = npc->baseTemplateForm->As<RE::TESNPC>();
	RE::TESLevCharacter* lvl = npc->baseTemplateForm->As<RE::TESLevCharacter>();
	if (tplt) {
		return IsLeveledChar(tplt);
	} else if (lvl)
		return false;
	else
		;  //logger::info("template invalid");
	return false;
}

Distribution::NPCTPLTInfo Distribution::ExtractTemplateInfo(RE::TESLevCharacter* lvl)
{
	if (lvl == nullptr)
		return Distribution::NPCTPLTInfo{};
	// just try to grab the first entry of the leveled list, since they should all share
	// factions 'n stuff
	if (lvl->entries.size() > 0) {
		RE::TESForm* entry = lvl->entries[0].form;
		RE::TESNPC* tplt = entry->As<RE::TESNPC>();
		RE::TESLevCharacter* lev = entry->As<RE::TESLevCharacter>();
		if (tplt)
			return ExtractTemplateInfo(tplt);
		else if (lev)
			return ExtractTemplateInfo(lev);
		else
			;  //logger::info("template invalid");
	}
	return Distribution::NPCTPLTInfo{};
}

Distribution::NPCTPLTInfo Distribution::ExtractTemplateInfo(RE::TESNPC* npc)
{
	Distribution::NPCTPLTInfo info;
	if (npc == nullptr)
		return info; 
	if (npc->baseTemplateForm == nullptr) {
		// we are at the base, so do the main work
		info.tpltrace = npc->GetRace();
		info.tpltstyle = npc->combatStyle;
		info.tpltclass = npc->npcClass;
		for (uint32_t i = 0; i < npc->numKeywords; i++) {
			if (npc->keywords[i])
				info.tpltkeywords.push_back(npc->keywords[i]);
		}
		for (uint32_t i = 0; i < npc->factions.size(); i++) {
			if (npc->factions[i].faction)
				info.tpltfactions.push_back(npc->factions[i].faction);
		}
		return info;
	}
	RE::TESNPC* tplt = npc->baseTemplateForm->As<RE::TESNPC>();
	RE::TESLevCharacter* lev = npc->baseTemplateForm->As<RE::TESLevCharacter>();
	Distribution::NPCTPLTInfo tpltinfo;
	if (tplt) {
		// get info about template and then integrate into our local information according to what we use
		tpltinfo = ExtractTemplateInfo(tplt);
	} else if (lev) {
		tpltinfo = ExtractTemplateInfo(lev);
	} else {
		//logger::info("template invalid");
	}

	if (npc->actorData.templateUseFlags & RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG::kFactions) {
		info.tpltfactions = tpltinfo.tpltfactions;
	} else {
		for (uint32_t i = 0; i < npc->factions.size(); i++) {
			if (npc->factions[i].faction)
				info.tpltfactions.push_back(npc->factions[i].faction);
		}
	}
	if (npc->actorData.templateUseFlags & RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG::kKeywords) {
		info.tpltkeywords = tpltinfo.tpltkeywords;
	} else {
		for (uint32_t i = 0; i < npc->numKeywords; i++) {
			if (npc->keywords[i])
				info.tpltkeywords.push_back(npc->keywords[i]);
		}
	}
	if (npc->actorData.templateUseFlags & RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG::kTraits) {
		// race
		info.tpltrace = tpltinfo.tpltrace;
	} else {
		info.tpltrace = npc->GetRace();
	}
	if (npc->actorData.templateUseFlags & RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG::kStats) {
		// class
		info.tpltclass = tpltinfo.tpltclass;
	} else {
		info.tpltclass = npc->npcClass;
	}
	if (npc->actorData.templateUseFlags & RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG::kAIData) {
		// combatstyle
		info.tpltstyle = tpltinfo.tpltstyle;
	} else {
		info.tpltstyle = npc->combatStyle;
	}
	return info;
}

std::tuple<uint64_t, ItemStrength, ItemType> Settings::ClassifyItem(RE::AlchemyItem* item)
{
	LOG_1("{}[Settings] [ClassifyItem]");
	RE::EffectSetting* sett = nullptr;
	if ((item->avEffectSetting) == nullptr && item->effects.size() == 0) {
		return { 0, ItemStrength::kStandard, ItemType::kFood };
	}
	// we look at max 4 effects
	RE::ActorValue av[4]{
		RE::ActorValue::kAlchemy,
		RE::ActorValue::kAlchemy,
		RE::ActorValue::kAlchemy,
		RE::ActorValue::kAlchemy
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
	// we will not abort the loop, since the number of effects on one item is normally very
	// limited, so we don't have much iterations
	if (item->effects.size() > 0) {
		for (uint32_t i = 0; i < item->effects.size() && i < 4; i++) {
			sett = item->effects[i]->baseEffect;
			// just retrieve the effects, we will analyze them later
			if (sett) {
				av[i] = sett->data.primaryAV;
				mag[i] = item->effects[i]->effectItem.magnitude;
				dur[i] = item->effects[i]->effectItem.duration;
				// we only need this for magnitude calculations, so its not used as cooldown
				if (dur[i] == 0)
					dur[i] = 1;
			}
		}
	} else {
		RE::MagicItem::SkillUsageData err;
		item->GetSkillUsageData(err);
		switch (item->avEffectSetting->data.primaryAV) {
		case RE::ActorValue::kHealth:
			av[0] = item->avEffectSetting->data.primaryAV;
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		case RE::ActorValue::kMagicka:
			av[0] = item->avEffectSetting->data.primaryAV;
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		case RE::ActorValue::kStamina:
			av[0] = item->avEffectSetting->data.primaryAV;
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		}
	}
	// analyze the effect types
	uint64_t alch = static_cast<uint64_t>(AlchemyEffect::kNone);
	ItemStrength str = ItemStrength::kWeak;
	float maxmag = 0;
	for (int i = 0; i < 4; i++) {
		if (dur[i] > 10)
			dur[i] = 10;
		switch (av[i]) {
		case RE::ActorValue::kHealth:
			alch |= static_cast<uint64_t>(AlchemyEffect::kHealth);
			if (mag[i] * dur[i] > maxmag)
				maxmag = mag[i] * dur[i];
			break;
		case RE::ActorValue::kMagicka:
			alch |= static_cast<uint64_t>(AlchemyEffect::kMagicka);
			if (mag[i] * dur[i] > maxmag)
				maxmag = mag[i] * dur[i];
			break;
		case RE::ActorValue::kStamina:
			alch |= static_cast<uint64_t>(AlchemyEffect::kStamina);
			if (mag[i] * dur[i] > maxmag)
				maxmag = mag[i] * dur[i];
			break;
		case RE::ActorValue::kOneHanded:
		case RE::ActorValue::kOneHandedModifier:
		case RE::ActorValue::kOneHandedPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kOneHanded);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kTwoHanded:
		case RE::ActorValue::kTwoHandedModifier:
		case RE::ActorValue::kTwoHandedPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kTwoHanded);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kArchery:
		case RE::ActorValue::kMarksmanModifier:
		case RE::ActorValue::kMarksmanPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kArchery);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kBlock:
		case RE::ActorValue::kBlockModifier:
		case RE::ActorValue::kBlockPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kBlock);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		//case RE::ActorValue::kSmithing:
		//	break;
		case RE::ActorValue::kHeavyArmor:
		case RE::ActorValue::kHeavyArmorModifier:
		case RE::ActorValue::kHeavyArmorPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kHeavyArmor);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kLightArmor:
		case RE::ActorValue::kLightArmorModifier:
		case RE::ActorValue::kLightArmorSkillAdvance:
			alch |= static_cast<uint64_t>(AlchemyEffect::kLightArmor);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kPickpocket:
		case RE::ActorValue::kPickpocketModifier:
		case RE::ActorValue::kPickpocketPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kPickpocket);
			break;
		case RE::ActorValue::kLockpicking:
		case RE::ActorValue::kLockpickingModifier:
		case RE::ActorValue::kLockpickingPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kLockpicking);
			break;
		case RE::ActorValue::kSneak:
		case RE::ActorValue::kSneakingModifier:
		case RE::ActorValue::kSneakingPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kSneak);
			break;
		//case RE::ActorValue::kAlchemy:
		//case RE::ActorValue::kAlchemyModifier:
		//case RE::ActorValue::kAlchemyPowerModifier:
		//	break;
		//case RE::ActorValue::kSpeech:
		//case RE::ActorValue::kSpeechcraftModifier:
		//case RE::ActorValue::kSpeechcraftPowerModifier:
		//	break;
		case RE::ActorValue::kAlteration:
		case RE::ActorValue::kAlterationModifier:
		case RE::ActorValue::kAlterationPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kAlteration);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kConjuration:
		case RE::ActorValue::kConjurationModifier:
		case RE::ActorValue::kConjurationPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kConjuration);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kDestruction:
		case RE::ActorValue::kDestructionModifier:
		case RE::ActorValue::kDestructionPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kDestruction);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kIllusion:
		case RE::ActorValue::kIllusionModifier:
		case RE::ActorValue::kIllusionPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kIllusion);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kRestoration:
		case RE::ActorValue::kRestorationModifier:
		case RE::ActorValue::kRestorationPowerModifier:
			alch |= static_cast<uint64_t>(AlchemyEffect::kRestoration);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
			//case RE::ActorValue::kEnchanting:
			//case RE::ActorValue::kEnchantingModifier:
			//case RE::ActorValue::kEnchantingPowerModifier:
			//	break;
		case RE::ActorValue::kHealRate:
			alch |= static_cast<uint64_t>(AlchemyEffect::kHealRate);
			if (mag[i] > maxmag)
				maxmag = mag[i];
			break;
		case RE::ActorValue::kMagickaRate:
			alch |= static_cast<uint64_t>(AlchemyEffect::kMagickaRate);
			if (mag[i] > maxmag)
				maxmag = mag[i];
			break;
		case RE::ActorValue::KStaminaRate:
			alch |= static_cast<uint64_t>(AlchemyEffect::kStaminaRate);
			if (mag[i] > maxmag)
				maxmag = mag[i];
			break;
		case RE::ActorValue::kSpeedMult:
			alch |= static_cast<uint64_t>(AlchemyEffect::kSpeedMult);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		//case RE::ActorValue::kInventoryWeight:
		//	break;
		//case RE::ActorValue::kCarryWeight:
		//	break;
		case RE::ActorValue::kCriticalChance:
			alch |= static_cast<uint64_t>(AlchemyEffect::kCriticalChance);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kMeleeDamage:
			alch |= static_cast<uint64_t>(AlchemyEffect::kMeleeDamage);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kUnarmedDamage:
			alch |= static_cast<uint64_t>(AlchemyEffect::kUnarmedDamage);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kDamageResist:
			alch |= static_cast<uint64_t>(AlchemyEffect::kDamageResist);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kPoisonResist:
			alch |= static_cast<uint64_t>(AlchemyEffect::kPoisonResist);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kResistFire:
			alch |= static_cast<uint64_t>(AlchemyEffect::kResistFire);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kResistShock:
			alch |= static_cast<uint64_t>(AlchemyEffect::kResistShock);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kResistFrost:
			alch |= static_cast<uint64_t>(AlchemyEffect::kResistFrost);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kResistMagic:
			alch |= static_cast<uint64_t>(AlchemyEffect::kResistMagic);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kResistDisease:
			alch |= static_cast<uint64_t>(AlchemyEffect::kResistDisease);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kParalysis:
			alch |= static_cast<uint64_t>(AlchemyEffect::kParalysis);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kInvisibility:
			alch |= static_cast<uint64_t>(AlchemyEffect::kInvisibility);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		//case RE::ActorValue::kNightEye:
		//	break;
		//case RE::ActorValue::kDetectLifeRange:
		//	break;
		//case RE::ActorValue::kWaterBreathing:
		//	break;
		//case RE::ActorValue::kWaterWalking:
		//	break;
		case RE::ActorValue::kWeaponSpeedMult:
		case RE::ActorValue::kLeftWeaponSpeedMultiply:
			alch |= static_cast<uint64_t>(AlchemyEffect::kWeaponSpeedMult);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kBowSpeedBonus:
			alch |= static_cast<uint64_t>(AlchemyEffect::kBowSpeed);
			break;
		case RE::ActorValue::kAttackDamageMult:
			alch |= static_cast<uint64_t>(AlchemyEffect::kAttackDamageMult);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kHealRateMult:
			alch |= static_cast<uint64_t>(AlchemyEffect::kHealRateMult);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kMagickaRateMult:
			alch |= static_cast<uint64_t>(AlchemyEffect::kMagickaRateMult);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kStaminaRateMult:
			alch |= static_cast<uint64_t>(AlchemyEffect::kStaminaRateMult);
			//if (mag[i] < maxmag)
			//	maxmag = mag[i];
			break;
		case RE::ActorValue::kAggresion:
			alch |= static_cast<uint64_t>(AlchemyEffect::kFrenzy);
			break;
		case RE::ActorValue::kConfidence:
			alch |= static_cast<uint64_t>(AlchemyEffect::kFear);
			break;
		case RE::ActorValue::kReflectDamage:
			alch |= static_cast<uint64_t>(AlchemyEffect::kReflectDamage);
			break;
		}
	}
	if (std::string(item->GetName()).find(std::string("Weak")) != std::string::npos)
		str = ItemStrength::kWeak;
	else if (std::string(item->GetName()).find(std::string("Standard")) != std::string::npos)
		str = ItemStrength::kStandard;
	else if (std::string(item->GetName()).find(std::string("Potent")) != std::string::npos)
		str = ItemStrength::kPotent;
	else if (maxmag == 0)
		str = ItemStrength::kStandard;
	else if (maxmag <= NUPSettings::_MaxMagnitudeWeak)
		str = ItemStrength::kWeak;
	else if (maxmag <= NUPSettings::_MaxMagnitudeStandard)
		str = ItemStrength::kStandard;
	else if (maxmag <= NUPSettings::_MaxMagnitudePotent)
		str = ItemStrength::kPotent;
	else
		str = ItemStrength::kInsane;

	// if the potion is a blood potion it should only ever appear on vampires, no the
	// effects are overriden to AlchemyEffect::kBlood
	if (std::string(item->GetName()).find(std::string("Blood")) != std::string::npos &&
		std::string(item->GetName()).find(std::string("Potion")) != std::string::npos) {
		alch = static_cast<uint64_t>(AlchemyEffect::kBlood);
	}

	ItemType t = ItemType::kPotion;
	if (item->IsFood())
		t = ItemType::kFood;
	else if (item->IsPoison())
		t = ItemType::kPoison;

	return { alch, str, t };
}

void Settings::Interfaces::RequestAPIs()
{
	LOG_1("{}[Settings] [RequestAPIs]");
	if (!nupinter) {
		nupinter = reinterpret_cast<NPCsUsePotions::NUPInterface*>(NPCsUsePotions::RequestPluginAPI());
		if (nupinter)
			logger::info("[Settings] [RequestAPIs] loaded NPCsUsePotions api");
		else
			logger::info("[Settings] [RequestAPIs] failed to obtain NPCsUsePotions api");
	}
}

#pragma endregion
