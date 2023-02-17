#pragma once

#include <fstream>
#include <iostream>
#include <type_traits>
#include <utility>
#include <string_view>
#include <chrono>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <time.h>
#include <random>
#include <tuple>
#include <vector>
#include <string.h>
#include<thread>
#include <forward_list>
#include <semaphore>
#include <limits>

#include "SimpleIni.h"
#include "ActorInfo.h"
#include "Logging.h"
#include "Interface/NUPInterface.h"
#include "AlchemyEffect.h"


typedef uint64_t AlchemyEffectBase;
typedef uint32_t FormID;
typedef int8_t Number;
typedef int32_t Chance;

class Settings
{
public:
	/// <summary>
	/// Name of this plugin
	/// </summary>
	static inline std::string PluginName = "AlchemyExpansion.esp";

	/// <summary>
	/// Contains values used for compatibility
	/// </summary>
	class Compatibility
	{
	public:
	};

	/// <summary>
	/// Supported types of Items
	/// </summary>
	enum class ItemType
	{
		kPoison = 1,
		kPotion = 2,
		kFood = 4,
		kFortifyPotion = 8,
	};

	/// <summary>
	/// Loads the distribution configuration
	/// </summary>
	static void LoadDistrConfigNUP();

	/// <summary>
	/// Loads the distribution configuration
	/// </summary>
	static void LoadDistrConfigAlchExt();

	/// <summary>
	/// loads game objects
	/// </summary>
	static void LoadGameObjects();

	class NUPSettings
	{
	public:
		static inline int _MaxDuration = 10000;
		static inline int _MaxFortifyDuration = 60000;

		//general
		static inline long _maxPotionsPerCycle = 2;
		static inline long _cycletime = 500;
		static inline bool _DisableEquipSounds = false;

		// features
		static inline bool _featMagickaRestoration = true;               // enables automatic usage of magicka potions
		static inline bool _featStaminaRestoration = true;               // enables automatic usage of stamina potions
		static inline bool _featHealthRestoration = true;                // enables automatic usage of health potions
		static inline bool _playerRestorationEnabled = false;            // enables automatic usage of potions for the player
		static inline bool _featUsePoisons = true;                       // enables automatic usage of poisons for npcs
		static inline bool _featUseFortifyPotions = true;                // enables automatic usage of fortify potions for npcs
		static inline bool _featUseFood = true;                          // enables automatic usage of food for npcs
		static inline bool _playerUsePoisons = false;                    // enables automatic usage of poisons for player
		static inline bool _playerUseFortifyPotions = false;             // enables automatic usage of fortify potions for player
		static inline bool _featDistributePoisons = true;                // player is excluded from distribution options, as well as followers
		static inline bool _featDistributePotions = true;                // player is excluded from distribution options, as well as followers
		static inline bool _featDistributeFortifyPotions = true;         // player is excluded from distribution options, as well as followers
		static inline bool _featDistributeFood = true;                   // player is excluded from distribution options, as well as followers
		static inline bool _featUseDeathItems = true;                    // the npc will be given potions that may appear in their deathItems if available
		static inline bool _featRemoveItemsOnDeath = true;               // remove unused items on death, if activated chances for removal can be set
		static inline bool _featDisableItemUsageWhileStaggered = false;  // disables potion and poison usage while the npc is staggered

		// debug
		static inline bool EnableLog = false;
		static inline int LogLevel = 0;      // 0 - only highest level
											 // 1 - highest to layer 1 function logging
											 // 2 - highest to layer 2 function logging
											 // 3 - highest to layer 3 function logging
		static inline int ProfileLevel = 0;  // 0 - highest level only
											 // 1 - highest and layer 1
											 // 2 - highest and layer 2
		static inline bool EnableProfiling = false;
		static inline bool _CalculateCellRules = false;

		// distribution
		static inline int _LevelEasy = 20;       // only distribute "weak" potions and poisons
		static inline int _LevelNormal = 35;     // may distribute "standard" potions and poisons
		static inline int _LevelDifficult = 50;  // may distribute "potent" potions and poisons
		static inline int _LevelInsane = 70;     // may have Insane tear potions

		static inline bool _GameDifficultyScaling = false;  // ties the strength of the actors not to levels, but the game difficulty

		static inline int _MaxMagnitudeWeak = 30;      // max potion / poison magnitude to be considered "weak"
		static inline int _MaxMagnitudeStandard = 60;  // max potion / poison magnitude to be considered "standard"
		static inline int _MaxMagnitudePotent = 150;   // max potion / poison magnitude to be considered "potent"
													   // anything above this won't be distributed

		// potion usage
		static inline float _healthThreshold = 0.5f;
		static inline float _magickaThreshold = 0.5f;
		static inline float _staminaThreshold = 0.5f;
		static inline int _UsePotionChance = 100;  // Chance that a potion will be used when appropiate

		// poison usage
		static inline float _EnemyLevelScalePlayerLevel = 0.8f;  // how high the level of an enemy must be for followers to use poisons
		static inline int _EnemyNumberThreshold = 5;             // how many npcs must be fighting, for followers to use poisons regardless of the enemies level
		static inline int _UsePoisonChance = 100;                // Chance that a poison will be used when possible

		// fortify potions
		static inline float _EnemyLevelScalePlayerLevelFortify = 0.8f;  // how high the level of an enemy must be for followers to use fortify potions
		static inline int _EnemyNumberThresholdFortify = 5;             // how many npcs must be fighting, for followers to use fortify potions regardless of the enemies level
		static inline int _UseFortifyPotionChance = 100;                // Chance that a fortify potion will be used when possible

		// removal
		static inline int _ChanceToRemoveItem = 90;  // chance for an item to be removed
		static inline int _MaxItemsLeft = 2;         // maximum number of items that may remain, from those to be removed

		// intern
		static inline bool _CheckActorsWithoutRules = false;  // checks for actors which do not have any rules, and prints their information to the, logfile
		static inline bool _Test = false;
	};

	class AlchExtSettings
	{
	public:
		// general
		static inline int CycleTime = 10000;

		static inline float TickLength = 0.005f;

		// debug
		static inline bool EnableLog = false;
		static inline int LogLevel = 0;      // 0 - only highest level
											 // 1 - highest to layer 1 function logging
											 // 2 - highest to layer 2 function logging
											 // 3 - highest to layer 3 function logging
		static inline int ProfileLevel = 0;  // 0 - highest level only
											 // 1 - highest and layer 1
											 // 2 - highest and layer 2
		static inline bool EnableProfiling = false;

		// compatibility
		static inline bool _CompatibilityCACO = false;        // automatic
		static inline bool _CompatibilityApothecary = false;  // automatic

		// game settings
		static inline bool _ignoreVampireBaseImmunity = false; // ignores the vampires base immunity to diseases
		static inline bool _ignoreWerewolfBaseImmunity = false; // ignores the werewolfs base immunity to diseases
		static inline bool _ignoreDiseaseResistance = false; // ignores all disease resistance and immunities
		static inline float _particleRange = 150.0f;
		static inline bool _ignoreTimeAdvancementConstraint = false; // ignores the time constraint on advancing stages
	};

	class Interfaces
	{
	public:
		static inline NPCsUsePotions::NUPInterface* nupinter = nullptr;

		static void RequestAPIs();
	};

	class AlchExtRuntime
	{
	public:
		static inline bool initialised = false;

		class HarvestItem
		{
		public:
			RE::TESBoundObject* object;
			Number num;
			Chance chance;
		};

	private:
		static inline std::unordered_map<FormID, std::vector<HarvestItem*>> _dummyMap1;

		/// <summary>
		/// maps harvested items, to items that are given additionally to the initial harvest
		/// </summary>
		static inline std::unordered_map<FormID, std::vector<HarvestItem*>> _harvestMap;

	public:
		/// <summary>
		/// return the harvested item - additionally harvest items map
		/// </summary>
		/// <returns></returns>
		static std::unordered_map<FormID, std::vector<HarvestItem*>>* harvestMap() { return initialised ? &_harvestMap : &_dummyMap1; }
	};

public:
	
	class GameObj
	{
	public:
		static inline RE::BGSKeyword* VendorItemPotion;
		static inline RE::BGSKeyword* VendorItemFood;
		static inline RE::BGSKeyword* VendorItemFoodRaw;
		static inline RE::BGSKeyword* VendorItemPoison;
		static inline RE::BGSKeyword* ActorTypeDwarven;
		static inline RE::BGSKeyword* Vampire;

		static inline RE::TESFaction* CurrentFollowerFaction;
		static inline RE::TESFaction* CurrentHirelingFaction;
		static inline RE::TESFaction* WerewolfFaction;
		static inline RE::TESFaction* CreatureFaction;

		static inline RE::SpellItem* WerewolfImmunity;
	};

	/// <summary>
	/// Loads the plugin configuration
	/// </summary>
	static void LoadNUP();

	/// <summary>
	/// Loads settings for AlchExt plugin
	/// </summary>
	static void LoadAlchExt();

	static void SaveAlchExt();

	/// <summary>
	/// classifies a single item based on its effects
	/// </summary>
	/// <param name="item"></param>
	/// <returns></returns>
	static std::tuple<uint64_t, ItemStrength, ItemType> ClassifyItem(RE::AlchemyItem* item);

	/// <summary>
	/// returns all item from [list] with the alchemy effect [effect]
	/// </summary>
	/// <param name="list"></param>
	/// <param name="effect"></param>
	/// <returns></returns>
	static std::vector<RE::AlchemyItem*> GetMatchingItems(std::list<std::pair<uint64_t, RE::AlchemyItem*>>& list, uint64_t effect)
	{
		std::vector<RE::AlchemyItem*> ret;
		for (auto entry : list) {
			if ((std::get<0>(entry) & effect) > 0)
				ret.push_back(std::get<1>(entry));
		}
		return ret;
	}
};
