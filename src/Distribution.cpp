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
#include "Utility.h"

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


#pragma region Distribution

/// <summary>
/// returns wether an npc is excluded from item distribution
/// </summary>
/// <param name="actor">ActorRef to check</param>
/// <returns></returns>
bool Distribution::ExcludedNPC(RE::Actor* actor)
{
	// skip fucking deleted references
	if (actor->formFlags & RE::TESForm::RecordFlags::kDeleted)
		return true;
	bool ret = Distribution::excludedNPCs()->contains(actor->GetFormID()) ||
	           actor->IsInFaction(Settings::GameObj::CurrentFollowerFaction) ||
	           actor->IsInFaction(Settings::GameObj::CurrentHirelingFaction) ||
	           (Distribution::excludedNPCs()->contains(actor->GetActorBase()->GetFormID())) ||
	           actor->IsGhost();
	// if the actor has an exclusive rule then this goes above Race, Faction and Keyword exclusions
	if (!Distribution::npcMap()->contains(actor->GetFormID()) && ret == false) {
		auto base = actor->GetActorBase();
		for (uint32_t i = 0; i < base->numKeywords; i++) {
			if (base->keywords[i])
				ret |= Distribution::excludedAssoc()->contains(base->keywords[i]->GetFormID());
		}
		for (uint32_t i = 0; i < base->factions.size(); i++) {
			if (base->factions[i].faction)
				ret |= Distribution::excludedAssoc()->contains(base->factions[i].faction->GetFormID());
		}
		auto race = actor->GetRace();
		if (race) {
			ret |= Distribution::excludedAssoc()->contains(race->GetFormID());
			for (uint32_t i = 0; i < race->numKeywords; i++) {
				ret |= Distribution::excludedAssoc()->contains(race->keywords[i]->GetFormID());
			}
		}
	}
	return ret;
}
/// <summary>
/// returns wether an npc is excluded from item distribution
/// </summary>
/// <param name="npc">ActorBase to check</param>
/// <returns></returns>
bool Distribution::ExcludedNPC(RE::TESNPC* npc)
{
	// skip fucking deleted references
	if (npc->formFlags & RE::TESForm::RecordFlags::kDeleted)
		return true;
	bool ret = (Distribution::excludedNPCs()->contains(npc->GetFormID())) ||
	           npc->IsInFaction(Settings::GameObj::CurrentFollowerFaction) ||
	           npc->IsInFaction(Settings::GameObj::CurrentHirelingFaction) ||
	           npc->IsGhost();
	// if the actor has an exclusive rule then this goes above Race, Faction and Keyword exclusions
	if (!Distribution::npcMap()->contains(npc->GetFormID()) && ret == false) {
		for (uint32_t i = 0; i < npc->numKeywords; i++) {
			if (npc->keywords[i])
				ret |= Distribution::excludedAssoc()->contains(npc->keywords[i]->GetFormID());
		}
		for (uint32_t i = 0; i < npc->factions.size(); i++) {
			if (npc->factions[i].faction)
				ret |= Distribution::excludedAssoc()->contains(npc->factions[i].faction->GetFormID());
		}
		auto race = npc->GetRace();
		if (race) {
			ret |= Distribution::excludedAssoc()->contains(race->GetFormID());
			for (uint32_t i = 0; i < race->numKeywords; i++) {
				ret |= Distribution::excludedAssoc()->contains(race->keywords[i]->GetFormID());
			}
		}
	}
	return ret;
}

Distribution::Rule* Distribution::CalcRule(RE::TESNPC* npc, ActorStrength& acs, ItemStrength& is, NPCTPLTInfo* tpltinfo, CustomItemStorage* custItems)
{
	// calc strength section
	if (Settings::NUPSettings::_GameDifficultyScaling) {
		// 0 novice, 1 apprentice, 2 adept, 3 expert, 4 master, 5 legendary
		auto diff = RE::PlayerCharacter::GetSingleton()->difficulty;
		if (diff == 0 || diff == 1) {
			acs = ActorStrength::Weak;
			is = ItemStrength::kWeak;
		} else if (diff == 2 || diff == 3) {
			acs = ActorStrength::Normal;
			is = ItemStrength::kStandard;
		} else if (diff == 4) {
			acs = ActorStrength::Powerful;
			is = ItemStrength::kPotent;
		} else {  // diff == 5
			acs = ActorStrength::Insane;
			is = ItemStrength::kInsane;
		}
	} else {
		// level not available for BaseActors

		/*
		// get level dependencies
		short lvl = actor->GetLevel();
		if (lvl <= NUPSettings::_LevelEasy) {
			acs = ActorStrength::Weak;
			is = ItemStrength::kWeak;
			// weak actor
		} else if (lvl <= NUPSettings::_LevelNormal) {
			acs = ActorStrength::Normal;
			is = ItemStrength::kStandard;
			// normal actor
		} else if (lvl <= NUPSettings::_LevelDifficult) {
			acs = ActorStrength::Powerful;
			is = ItemStrength::kPotent;
			// difficult actor
		} else if (lvl <= NUPSettings::_LevelInsane) {
			acs = ActorStrength::Insane;
			is = ItemStrength::kInsane;
			// insane actor
		} else {
			acs = ActorStrength::Boss;
			is = ItemStrength::kInsane;
			// boss actor
		}*/
	}

	// now calculate rule and on top get the boss override

	bool bossoverride = false;

	bool ruleoverride = false;
	bool baseexcluded = false;
	int prio = INT_MIN;

	Rule* rule = nullptr;
	// define general stuff
	auto style = npc->combatStyle;
	auto cls = npc->npcClass;
	auto race = npc->GetRace();

	// find rule in npc map
	// npc rules always have the highest priority
	auto itnpc = npcMap()->find(npc->GetFormID());
	if (itnpc != npcMap()->end()) {  // found the right rule!
		rule = itnpc->second;     // this can be null if the specific npc is excluded
		ruleoverride = true;
		prio = INT_MAX;
	}
	bossoverride |= bosses()->contains(npc->GetFormID());
	// get custom items
	if (custItems) {
		auto itc = customItems()->find(npc->GetFormID());
		if (itc != customItems()->end()) {
			auto vec = itc->second;
			for (int d = 0; d < vec.size(); d++) {
				custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
				custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
				custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
				custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
				custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
			}
		}
	}

	if (custItems == nullptr && ruleoverride && bossoverride) {
		goto SKIPNPC;
	}

	if (tpltinfo && tpltinfo->tpltrace)
		race = tpltinfo->tpltrace;
	// now that we didnt't find something so far, check the rest
	// this time all the priorities are the same
	if (!ruleoverride) {
		auto it = assocMap()->find(race->GetFormID());
		if (it != assocMap()->end())
			if (prio < std::get<0>(it->second)) {
				rule = std::get<1>(it->second);
				prio = std::get<0>(it->second);
			} else if (prio < std::get<1>(it->second)->rulePriority) {
				rule = std::get<1>(it->second);
				prio = std::get<1>(it->second)->rulePriority;
			}
		baseexcluded |= baselineExclusions()->contains(race->GetFormID());
		for (uint32_t i = 0; i < race->numKeywords; i++) {
			auto itr = assocMap()->find(race->keywords[i]->GetFormID());
			if (itr != assocMap()->end())
			{
				if (prio < std::get<0>(itr->second)) {
					rule = std::get<1>(itr->second);
					prio = std::get<0>(itr->second);
				} else if (prio < std::get<1>(itr->second)->rulePriority) {
					rule = std::get<1>(itr->second);
					prio = std::get<1>(itr->second)->rulePriority;
				}
				baseexcluded |= baselineExclusions()->contains(race->keywords[i]->GetFormID());
			}
		}
	}
	bossoverride |= bosses()->contains(npc->GetRace()->GetFormID());
	// get custom items
	if (custItems) {
		auto itc = customItems()->find(race->GetFormID());
		if (itc != customItems()->end()) {
			auto vec = itc->second;
			for (int d = 0; d < vec.size(); d++) {
				custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
				custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
				custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
				custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
				custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
			}
		}
		for (uint32_t i = 0; i < race->numKeywords; i++) {
			itc = customItems()->find(race->keywords[i]->GetFormID());
			if (itc != customItems()->end()) {
				auto vec = itc->second;
				for (int d = 0; d < vec.size(); d++) {
					custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
					custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
					custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
					custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
					custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
				}
			}
		}
	}

	if (custItems == nullptr && ruleoverride && bossoverride) {
		goto SKIPNPC;
	}

	// handle keywords
	for (unsigned int i = 0; i < npc->numKeywords; i++) {
		auto key = npc->keywords[i];
		if (key) {
			if (!ruleoverride) {
				auto it = assocMap()->find(key->GetFormID());
				if (it != assocMap()->end())
					if (prio < std::get<0>(it->second)) {
						rule = std::get<1>(it->second);
						prio = std::get<0>(it->second);
					} else if (prio < std::get<1>(it->second)->rulePriority) {
						rule = std::get<1>(it->second);
						prio = std::get<1>(it->second)->rulePriority;
					}
				baseexcluded |= baselineExclusions()->contains(key->GetFormID());
			}
			bossoverride |= bosses()->contains(key->GetFormID());
			// get custom items
			if (custItems) {
				auto itc = customItems()->find(key->GetFormID());
				if (itc != customItems()->end()) {
					auto vec = itc->second;
					for (int d = 0; d < vec.size(); d++) {
						custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
						custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
						custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
						custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
						custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
					}
				}
			}
		}
	}
	if (tpltinfo) {
		for (int i = 0; i < tpltinfo->tpltkeywords.size(); i++) {
			if (tpltinfo->tpltkeywords[i]) {
				if (!ruleoverride) {
					auto it = assocMap()->find(tpltinfo->tpltkeywords[i]->GetFormID());
					if (it != assocMap()->end())
						if (prio < std::get<0>(it->second)) {
							rule = std::get<1>(it->second);
							prio = std::get<0>(it->second);
						} else if (prio < std::get<1>(it->second)->rulePriority) {
							rule = std::get<1>(it->second);
							prio = std::get<1>(it->second)->rulePriority;
						}
					baseexcluded |= baselineExclusions()->contains(tpltinfo->tpltkeywords[i]->GetFormID());
				}
				bossoverride |= bosses()->contains(tpltinfo->tpltkeywords[i]->GetFormID());
				// get custom items
				if (custItems) {
					auto itc = customItems()->find(tpltinfo->tpltkeywords[i]->GetFormID());
					if (itc != customItems()->end()) {
						auto vec = itc->second;
						for (int d = 0; d < vec.size(); d++) {
							custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
							custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
							custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
							custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
							custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
						}
					}
				}
			}
		}
	}
	if (custItems == nullptr && ruleoverride && bossoverride) {
		goto SKIPNPC;
	}

	// handle factions
	for (uint32_t i = 0; i < npc->factions.size(); i++) {
		if (!ruleoverride) {
			auto it = assocMap()->find(npc->factions[i].faction->GetFormID());
			if (it != assocMap()->end()) {
				if (prio < std::get<0>(it->second)) {
					rule = std::get<1>(it->second);
					prio = std::get<0>(it->second);
				} else if (prio < std::get<1>(it->second)->rulePriority) {
					rule = std::get<1>(it->second);
					prio = std::get<1>(it->second)->rulePriority;
				}
			}
			baseexcluded |= baselineExclusions()->contains(npc->factions[i].faction->GetFormID());
		}
		bossoverride |= bosses()->contains(npc->factions[i].faction->GetFormID());
		if (custItems) {
			auto itc = customItems()->find(npc->factions[i].faction->GetFormID());
			if (itc != customItems()->end()) {
				auto vec = itc->second;
				for (int d = 0; d < vec.size(); d++) {
					custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
					custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
					custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
					custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
					custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
				}
			}
		}
	}
	if (tpltinfo) {
		for (int i = 0; i < tpltinfo->tpltfactions.size(); i++) {
			if (tpltinfo->tpltfactions[i]) {
				if (!ruleoverride) {
					auto it = assocMap()->find(tpltinfo->tpltfactions[i]->GetFormID());
					if (it != assocMap()->end()) {
						if (prio < std::get<0>(it->second)) {
							rule = std::get<1>(it->second);
							prio = std::get<0>(it->second);
						} else if (prio < std::get<1>(it->second)->rulePriority) {
							rule = std::get<1>(it->second);
							prio = std::get<1>(it->second)->rulePriority;
						}
					}
					baseexcluded |= baselineExclusions()->contains(tpltinfo->tpltfactions[i]->GetFormID());
				}
				bossoverride |= bosses()->contains(tpltinfo->tpltfactions[i]->GetFormID());
				if (custItems) {
					auto itc = customItems()->find(tpltinfo->tpltfactions[i]->GetFormID());
					if (itc != customItems()->end()) {
						auto vec = itc->second;
						for (int d = 0; d < vec.size(); d++) {
							custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
							custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
							custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
							custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
							custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
						}
					}
				}
			}
		}
	}

	if (custItems == nullptr && (bossoverride && ruleoverride || ruleoverride))
		goto SKIPNPC;

	// handle classes
	if (tpltinfo && tpltinfo->tpltclass)
		cls = tpltinfo->tpltclass;
	if (cls) {
		if (!ruleoverride) {
			auto it = assocMap()->find(cls->GetFormID());
			if (it != assocMap()->end()) {
				if (prio < std::get<0>(it->second)) {
					rule = std::get<1>(it->second);
					prio = std::get<0>(it->second);
				} else if (prio < std::get<1>(it->second)->rulePriority) {
					rule = std::get<1>(it->second);
					prio = std::get<1>(it->second)->rulePriority;
				}
			}
		}
		if (custItems) {
			auto itc = customItems()->find(cls->GetFormID());
			if (itc != customItems()->end()) {
				auto vec = itc->second;
				for (int d = 0; d < vec.size(); d++) {
					custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
					custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
					custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
					custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
					custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
				}
			}
		}
	}
	// handle combat styles
	if (tpltinfo && tpltinfo->tpltstyle)
		style = tpltinfo->tpltstyle;
	if (style) {
		if (!ruleoverride) {
			auto it = assocMap()->find(style->GetFormID());
			if (it != assocMap()->end()) {
				if (prio < std::get<0>(it->second)) {
					rule = std::get<1>(it->second);
					prio = std::get<0>(it->second);
				} else if (prio < std::get<1>(it->second)->rulePriority) {
					rule = std::get<1>(it->second);
					prio = std::get<1>(it->second)->rulePriority;
				}
			}
		}
		if (custItems) {
			auto itc = customItems()->find(style->GetFormID());
			if (itc != customItems()->end()) {
				auto vec = itc->second;
				for (int d = 0; d < vec.size(); d++) {
					custItems->items.insert(custItems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
					custItems->potions.insert(custItems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
					custItems->poisons.insert(custItems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
					custItems->fortify.insert(custItems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
					custItems->food.insert(custItems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
				}
			}
		}
	}

SKIPNPC:

	if (bossoverride)
		acs = ActorStrength::Boss;

	if (rule) {
		LOG1_1("{}[Distribution] [CalcRuleBase] rule found: {}", rule->ruleName);
		return rule;
	} else {
		// there are no rules!!!
		if (baseexcluded)
			return Distribution::emptyRule;
		LOG1_1("{}[Distribution] [CalcRuleBase] default rule found: {}", Distribution::defaultRule->ruleName);
		return Distribution::defaultRule;
	}
}

Distribution::Rule* Distribution::CalcRule(ActorInfo* acinfo, NPCTPLTInfo* tpltinfo)
{
	if (acinfo == nullptr)
		return emptyRule;
	// calc strength section
	if (Settings::NUPSettings::_GameDifficultyScaling) {
		// 0 novice, 1 apprentice, 2 adept, 3 expert, 4 master, 5 legendary
		auto diff = RE::PlayerCharacter::GetSingleton()->difficulty;
		if (diff == 0 || diff == 1) {
			acinfo->actorStrength = ActorStrength::Weak;
			acinfo->itemStrength = ItemStrength::kWeak;
		} else if (diff == 2 || diff == 3) {
			acinfo->actorStrength = ActorStrength::Normal;
			acinfo->itemStrength = ItemStrength::kStandard;
		} else if (diff == 4) {
			acinfo->actorStrength = ActorStrength::Powerful;
			acinfo->itemStrength = ItemStrength::kPotent;
		} else {  // diff == 5
			acinfo->actorStrength = ActorStrength::Insane;
			acinfo->itemStrength = ItemStrength::kInsane;
		}
	} else {
		// get level dependencies
		short lvl = acinfo->actor->GetLevel();
		if (lvl <= Settings::NUPSettings::_LevelEasy) {
			acinfo->actorStrength = ActorStrength::Weak;
			acinfo->itemStrength = ItemStrength::kWeak;
			// weak actor
		} else if (lvl <= Settings::NUPSettings::_LevelNormal) {
			acinfo->actorStrength = ActorStrength::Normal;
			acinfo->itemStrength = ItemStrength::kStandard;
			// normal actor
		} else if (lvl <= Settings::NUPSettings::_LevelDifficult) {
			acinfo->actorStrength = ActorStrength::Powerful;
			acinfo->itemStrength = ItemStrength::kPotent;
			// difficult actor
		} else if (lvl <= Settings::NUPSettings::_LevelInsane) {
			acinfo->actorStrength = ActorStrength::Insane;
			acinfo->itemStrength = ItemStrength::kInsane;
			// insane actor
		} else {
			acinfo->actorStrength = ActorStrength::Boss;
			acinfo->itemStrength = ItemStrength::kInsane;
			// boss actor
		}
	}
	//logger::info("rule 1");
	// now calculate rule and on top get the boss override

	bool bossoverride = false;

	bool ruleoverride = false;
	bool baseexcluded = false;
	int prio = INT_MIN;

	bool calccustitems = !acinfo->citems->calculated;

	auto base = acinfo->actor->GetActorBase();

	Rule* rule = nullptr;
	// define general stuff
	auto race = acinfo->actor->GetRace();
	//logger::info("rule 2");

	//std::vector<Rule*> rls;
	// find rule in npc map
	// npc rules always have the highest priority
	auto itnpc = npcMap()->find(acinfo->actor->GetFormID());
	if (itnpc != npcMap()->end()) {  // found the right rule!
		rule = itnpc->second;        // this can be null if the specific npc is excluded
		//logger::info("assign rule 1");
		ruleoverride = true;
		prio = INT_MAX;
	}
	bossoverride |= bosses()->contains(acinfo->actor->GetFormID());
	// get custom items
	if (calccustitems) {
		auto itc = customItems()->find(acinfo->actor->GetFormID());
		if (itc != customItems()->end()) {
			auto vec = itc->second;
			for (int d = 0; d < vec.size(); d++) {
				acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
				acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
				acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
				acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
				acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
			}
		}
	}
	//logger::info("rule 3");

	if (!calccustitems && ruleoverride && bossoverride) {
		goto SKIPActor;
	}

	// now also perform a check on the actor base
	if (!ruleoverride) {
		itnpc = npcMap()->find(acinfo->actor->GetActorBase()->GetFormID());
		if (itnpc != npcMap()->end()) {  // found the right rule!
			rule = itnpc->second;        // this can be null if the specific npc is excluded
			//logger::info("assign rule 2");
			ruleoverride = true;
			prio = INT_MAX;
		}
	}
	//logger::info("rule 4");
	bossoverride |= bosses()->contains(acinfo->actor->GetActorBase()->GetFormID());
	// get custom items
	if (calccustitems) {
		auto itc = customItems()->find(acinfo->actor->GetActorBase()->GetFormID());
		if (itc != customItems()->end()) {
			auto vec = itc->second;
			for (int d = 0; d < vec.size(); d++) {
				acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
				acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
				acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
				acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
				acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
			}
		}
	}

	if (!calccustitems && ruleoverride && bossoverride) {
		goto SKIPActor;
	}

	//logger::info("rule 5");

	if (tpltinfo && tpltinfo->tpltrace)
		race = tpltinfo->tpltrace;
	// now that we didnt't find something so far, check the rest
	// this time all the priorities are the same
	if (!ruleoverride) {
		//logger::info("rule 6");
		auto it = assocMap()->find(race->GetFormID());
		if (it != assocMap()->end())
			if (prio < std::get<0>(it->second)) {
				rule = std::get<1>(it->second);
				//logger::info("assign rule 3");
				prio = std::get<0>(it->second);
			} else if (prio < std::get<1>(it->second)->rulePriority) {
				rule = std::get<1>(it->second);
				//logger::info("assign rule 4");
				prio = std::get<1>(it->second)->rulePriority;
			}
		baseexcluded |= baselineExclusions()->contains(race->GetFormID());
		for (uint32_t i = 0; i < race->numKeywords; i++) {
			auto itr = assocMap()->find(race->keywords[i]->GetFormID());
			if (itr != assocMap()->end()) {
				if (prio < std::get<0>(itr->second)) {
					rule = std::get<1>(itr->second);
					//logger::info("assign rule 5 {} {} {}", Utility::GetHex((uintptr_t)std::get<1>(itr->second)), race->keywords[i]->GetFormEditorID(), Utility::GetHex(race->keywords[i]->GetFormID()));
					prio = std::get<0>(itr->second);
				} else if (prio < std::get<1>(itr->second)->rulePriority) {
					rule = std::get<1>(itr->second);
					//logger::info("assign rule 6");
					prio = std::get<1>(itr->second)->rulePriority;
				}
				baseexcluded |= baselineExclusions()->contains(race->keywords[i]->GetFormID());
			}
		}
	}
	//logger::info("rule 7");
	bossoverride |= bosses()->contains(base->GetRace()->GetFormID());
	// get custom items
	if (calccustitems) {
		auto itc = customItems()->find(race->GetFormID());
		if (itc != customItems()->end()) {
			auto vec = itc->second;
			for (int d = 0; d < vec.size(); d++) {
				acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
				acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
				acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
				acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
				acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
			}
		}
		for (uint32_t i = 0; i < race->numKeywords; i++) {
			itc = customItems()->find(race->keywords[i]->GetFormID());
			if (itc != customItems()->end()) {
				auto vec = itc->second;
				for (int d = 0; d < vec.size(); d++) {
					acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
					acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
					acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
					acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
					acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
				}
			}
		}
	}

	if (!calccustitems && ruleoverride && bossoverride) {
		goto SKIPActor;
	}

	//logger::info("rule 8");
	// handle keywords
	for (unsigned int i = 0; i < base->numKeywords; i++) {
		auto key = base->keywords[i];
		if (key) {
			if (!ruleoverride) {
				auto it = assocMap()->find(key->GetFormID());
				if (it != assocMap()->end())
					if (prio < std::get<0>(it->second)) {
						rule = std::get<1>(it->second);
						//logger::info("assign rule 7");
						prio = std::get<0>(it->second);
					} else if (prio < std::get<1>(it->second)->rulePriority) {
						rule = std::get<1>(it->second);
						//logger::info("assign rule 8");
						prio = std::get<1>(it->second)->rulePriority;
					}
				baseexcluded |= baselineExclusions()->contains(key->GetFormID());
			}
			bossoverride |= bosses()->contains(key->GetFormID());
			// get custom items
			if (calccustitems) {
				auto itc = customItems()->find(key->GetFormID());
				if (itc != customItems()->end()) {
					auto vec = itc->second;
					for (int d = 0; d < vec.size(); d++) {
						acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
						acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
						acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
						acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
						acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
					}
				}
			}
		}
	}
	if (tpltinfo) {
		//logger::info("rule 10");
		for (int i = 0; i < tpltinfo->tpltkeywords.size(); i++) {
			if (tpltinfo->tpltkeywords[i]) {
				if (!ruleoverride) {
					auto it = assocMap()->find(tpltinfo->tpltkeywords[i]->GetFormID());
					if (it != assocMap()->end())
						if (prio < std::get<0>(it->second)) {
							rule = std::get<1>(it->second);
							//logger::info("assign rule 9");
							prio = std::get<0>(it->second);
						} else if (prio < std::get<1>(it->second)->rulePriority) {
							rule = std::get<1>(it->second);
							//logger::info("assign rule 10");
							prio = std::get<1>(it->second)->rulePriority;
						}
					baseexcluded |= baselineExclusions()->contains(tpltinfo->tpltkeywords[i]->GetFormID());
				}
				bossoverride |= bosses()->contains(tpltinfo->tpltkeywords[i]->GetFormID());
				// get custom items
				if (calccustitems) {
					auto itc = customItems()->find(tpltinfo->tpltkeywords[i]->GetFormID());
					if (itc != customItems()->end()) {
						auto vec = itc->second;
						for (int d = 0; d < vec.size(); d++) {
							acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
							acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
							acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
							acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
							acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
						}
					}
				}
			}
		}
	}
	//logger::info("rule 11");
	if (!calccustitems && ruleoverride && bossoverride) {
		goto SKIPActor;
	}

	//logger::info("rule 12");
	// handle factions
	for (uint32_t i = 0; i < base->factions.size(); i++) {
		if (!ruleoverride) {
			auto it = assocMap()->find(base->factions[i].faction->GetFormID());
			if (it != assocMap()->end()) {
				if (prio < std::get<0>(it->second)) {
					rule = std::get<1>(it->second);
					//logger::info("assign rule 11");
					prio = std::get<0>(it->second);
				} else if (prio < std::get<1>(it->second)->rulePriority) {
					rule = std::get<1>(it->second);
					//logger::info("assign rule 12");
					prio = std::get<1>(it->second)->rulePriority;
				}
			}
			baseexcluded |= baselineExclusions()->contains(base->factions[i].faction->GetFormID());
		}
		bossoverride |= bosses()->contains(base->factions[i].faction->GetFormID());
		if (calccustitems) {
			auto itc = customItems()->find(base->factions[i].faction->GetFormID());
			if (itc != customItems()->end()) {
				auto vec = itc->second;
				for (int d = 0; d < vec.size(); d++) {
					acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
					acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
					acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
					acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
					acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
				}
			}
		}
	}
	if (tpltinfo) {
		//logger::info("rule 13");
		for (int i = 0; i < tpltinfo->tpltfactions.size(); i++) {
			if (tpltinfo->tpltfactions[i]) {
				if (!ruleoverride) {
					auto it = assocMap()->find(tpltinfo->tpltfactions[i]->GetFormID());
					if (it != assocMap()->end()) {
						if (prio < std::get<0>(it->second)) {
							rule = std::get<1>(it->second);
							//logger::info("assign rule 13");
							prio = std::get<0>(it->second);
						} else if (prio < std::get<1>(it->second)->rulePriority) {
							rule = std::get<1>(it->second);
							//logger::info("assign rule 14");
							prio = std::get<1>(it->second)->rulePriority;
						}
					}
					baseexcluded |= baselineExclusions()->contains(tpltinfo->tpltfactions[i]->GetFormID());
				}
				bossoverride |= bosses()->contains(tpltinfo->tpltfactions[i]->GetFormID());
				if (calccustitems) {
					auto itc = customItems()->find(tpltinfo->tpltfactions[i]->GetFormID());
					if (itc != customItems()->end()) {
						auto vec = itc->second;
						for (int d = 0; d < vec.size(); d++) {
							acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
							acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
							acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
							acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
							acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
						}
					}
				}
			}
		}
	}
	//logger::info("rule 14");
	if (!calccustitems && (ruleoverride && bossoverride || ruleoverride)) {
		goto SKIPActor;
	}

	// dont use tplt for class and combatstyle, since they may have been modified during runtime

	// handle classes
	if (base->npcClass) {
		//logger::info("rule 15");
		if (!ruleoverride) {
			auto it = assocMap()->find(base->npcClass->GetFormID());
			if (it != assocMap()->end()) {
				if (prio < std::get<0>(it->second)) {
					rule = std::get<1>(it->second);
					//logger::info("assign rule 15");
					prio = std::get<0>(it->second);
				} else if (prio < std::get<1>(it->second)->rulePriority) {
					rule = std::get<1>(it->second);
					//logger::info("assign rule 16");
					prio = std::get<1>(it->second)->rulePriority;
				}
			}
		}
		if (calccustitems) {
			auto itc = customItems()->find(base->npcClass->GetFormID());
			if (itc != customItems()->end()) {
				auto vec = itc->second;
				for (int d = 0; d < vec.size(); d++) {
					acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
					acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
					acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
					acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
					acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
				}
			}
		}
	}

	// handle combat styles
	if (base->combatStyle) {
		//logger::info("rule 16");
		if (!ruleoverride) {
			auto it = assocMap()->find(base->combatStyle->GetFormID());
			if (it != assocMap()->end()) {
				if (prio < std::get<0>(it->second)) {
					rule = std::get<1>(it->second);
					//logger::info("assign rule 17");
					prio = std::get<0>(it->second);
				} else if (prio < std::get<1>(it->second)->rulePriority) {
					rule = std::get<1>(it->second);
					//logger::info("assign rule 18");
					prio = std::get<1>(it->second)->rulePriority;
				}
			}
		}
		if (calccustitems) {
			auto itc = customItems()->find(base->combatStyle->GetFormID());
			if (itc != customItems()->end()) {
				auto vec = itc->second;
				for (int d = 0; d < vec.size(); d++) {
					acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
					acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
					acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
					acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
					acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
				}
			}
		}
	}

SKIPActor:

	if (bossoverride)
		acinfo->actorStrength = ActorStrength::Boss;
	//logger::info("rule 17");

	if (calccustitems) {
		auto itc = customItems()->find(0x0);
		if (itc != customItems()->end()) {
			auto vec = itc->second;
			for (int d = 0; d < vec.size(); d++) {
				acinfo->citems->items.insert(acinfo->citems->items.end(), vec[d]->items.begin(), vec[d]->items.end());
				acinfo->citems->potions.insert(acinfo->citems->potions.end(), vec[d]->potions.begin(), vec[d]->potions.end());
				acinfo->citems->poisons.insert(acinfo->citems->poisons.end(), vec[d]->poisons.begin(), vec[d]->poisons.end());
				acinfo->citems->fortify.insert(acinfo->citems->fortify.end(), vec[d]->fortify.begin(), vec[d]->fortify.end());
				acinfo->citems->food.insert(acinfo->citems->food.end(), vec[d]->food.begin(), vec[d]->food.end());
			}
		}
		acinfo->citems->calculated = true;
		acinfo->citems->CreateMaps();
	}
	acinfo->_boss = bossoverride;

	if (rule) {
		//logger::info("rule 18 {}", Utility::GetHex((uintptr_t)rule));
		LOG1_1("{}[Distribution] [CalcRuleBase] rule found: {}", rule->ruleName);
		return rule;
	} else {
		// there are no rules!!!
		//logger::info("rule 19");
		if (baseexcluded) {
			//logger::info("rule 20");
			return Distribution::emptyRule;
		}
		LOG1_1("{}[Distribution] [CalcRuleBase] default rule found: {}", Distribution::defaultRule->ruleName);
		return Distribution::defaultRule;
	}
}

#pragma endregion
