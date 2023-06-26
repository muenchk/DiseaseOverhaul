#include "Compatibility.h"
#include "Settings.h"
#include "Utility.h"

using Comp = Compatibility;

void Settings::NUPSettings::LoadNUP()
{
	LOG_1("{}[NUPSettings] [LoadNUP]");
	{
		constexpr auto path = L"Data/SKSE/Plugins/NPCsUsePotions.ini";

		bool Ultimateoptions = false;

		CSimpleIniA ini;

		ini.SetUnicode();
		ini.LoadFile(path);

		// new NUPSettings order version 3.x+
		{
			// potions
			Potions::_enableHealthRestoration = ini.GetBoolValue("Potions", "EnableHealthRestoration", Potions::_enableHealthRestoration);
			loginfo("[NUPSettings] {} {}", "Potions:           EnableHealthRestoration", std::to_string(Potions::_enableHealthRestoration));
			Potions::_enableMagickaRestoration = ini.GetBoolValue("Potions", "EnableMagickaRestoration", Potions::_enableMagickaRestoration);
			loginfo("[NUPSettings] {} {}", "Potions:           EnableMagickaRestoration", std::to_string(Potions::_enableMagickaRestoration));
			Potions::_enableStaminaRestoration = ini.GetBoolValue("Potions", "EnableStaminaRestoration", Potions::_enableStaminaRestoration);
			loginfo("[NUPSettings] {} {}", "Potions:           EnableStaminaRestoration", std::to_string(Potions::_enableStaminaRestoration));
			Potions::_AllowDetrimentalEffects = ini.GetBoolValue("Potions", "AllowDetrimentalEffects", Potions::_AllowDetrimentalEffects);
			loginfo("[NUPSettings] {} {}", "Potions:           AllowDetrimentalEffects", std::to_string(Potions::_AllowDetrimentalEffects));
			Potions::_HandleWeaponSheathedAsOutOfCombat = ini.GetBoolValue("Potions", "HandleWeaponSheathedAsOutOfCombat", Potions::_HandleWeaponSheathedAsOutOfCombat);
			loginfo("[NUPSettings] {} {}", "Potions:           HandleWeaponSheathedAsOutOfCombat", std::to_string(Potions::_HandleWeaponSheathedAsOutOfCombat));

			Potions::_healthThreshold = static_cast<float>(ini.GetDoubleValue("Potions", "HealthThresholdPercent", Potions::_healthThreshold));
			Potions::_healthThreshold = static_cast<float>(ini.GetDoubleValue("Potions", "HealthThresholdLowerPercent", Potions::_healthThreshold));
			if (Potions::_healthThreshold > 0.95f)
				Potions::_healthThreshold = 0.95f;
			loginfo("[NUPSettings] {} {}", "Potions:           HealthThresholdPercent", std::to_string(Potions::_healthThreshold));
			Potions::_magickaThreshold = static_cast<float>(ini.GetDoubleValue("Potions", "MagickaThresholdPercent", Potions::_magickaThreshold));
			Potions::_magickaThreshold = static_cast<float>(ini.GetDoubleValue("Potions", "MagickaThresholdLowerPercent", Potions::_magickaThreshold));
			if (Potions::_magickaThreshold > 0.95f)
				Potions::_magickaThreshold = 0.95f;
			loginfo("[NUPSettings] {} {}", "Potions:           MagickaThresholdPercent", std::to_string(Potions::_magickaThreshold));
			Potions::_staminaThreshold = static_cast<float>(ini.GetDoubleValue("Potions", "StaminaThresholdPercent", Potions::_staminaThreshold));
			Potions::_staminaThreshold = static_cast<float>(ini.GetDoubleValue("Potions", "StaminaThresholdLowerPercent", Potions::_staminaThreshold));
			if (Potions::_staminaThreshold > 0.95f)
				Potions::_staminaThreshold = 0.95f;
			loginfo("[NUPSettings] {} {}", "Potions:           StaminaThresholdPercent", std::to_string(Potions::_staminaThreshold));
			Potions::_UsePotionChance = static_cast<int>(ini.GetLongValue("Potions", "UsePotionChance", Potions::_UsePotionChance));
			loginfo("[NUPSettings] {} {}", "Potions:           UsePotionChance", std::to_string(Potions::_UsePotionChance));

			// poisons
			Poisons::_enablePoisons = ini.GetBoolValue("Poisons", "EnablePoisonUsage", Poisons::_enablePoisons);
			loginfo("[NUPSettings] {} {}", "Poisons:           EnablePoisonUsage", std::to_string(Poisons::_enablePoisons));
			Poisons::_AllowPositiveEffects = ini.GetBoolValue("Poisons", "AllowPositiveEffects", Poisons::_AllowPositiveEffects);
			loginfo("[NUPSettings] {} {}", "Poisons:           AllowPositiveEffects", std::to_string(Poisons::_AllowPositiveEffects));
			Poisons::_DontUseWithWeaponsSheathed = ini.GetBoolValue("Poisons", "DontUseWithWeaponsSheathed", Poisons::_DontUseWithWeaponsSheathed);
			loginfo("[NUPSettings] {} {}", "Poisons:           DontUseWithWeaponsSheathed", std::to_string(Poisons::_DontUseWithWeaponsSheathed));
			Poisons::_DontUseAgainst100PoisonResist = ini.GetBoolValue("Poisons", "DontUseAgainst100PoisonResist", Poisons::_DontUseAgainst100PoisonResist);
			loginfo("[NUPSettings] {} {}", "Poisons:           DontUseAgainst100PoisonResist", std::to_string(Poisons::_DontUseAgainst100PoisonResist));
			Poisons::_EnemyLevelScalePlayerLevel = static_cast<float>(ini.GetDoubleValue("Poisons", "EnemyLevelScalePlayerLevel", Poisons::_EnemyLevelScalePlayerLevel));
			loginfo("[NUPSettings] {} {}", "Poisons:           EnemyLevelScalePlayerLevel", std::to_string(Poisons::_EnemyLevelScalePlayerLevel));
			Poisons::_EnemyNumberThreshold = ini.GetLongValue("Poisons", "FightingNPCsNumberThreshold", Poisons::_EnemyNumberThreshold);
			loginfo("[NUPSettings] {} {}", "Poisons:           FightingNPCsNumberThreshold", std::to_string(Poisons::_EnemyNumberThreshold));
			Poisons::_UsePoisonChance = static_cast<int>(ini.GetLongValue("Poisons", "UsePoisonChance", Poisons::_UsePoisonChance));
			loginfo("[NUPSettings] {} {}", "Poisons:           UsePoisonChance", std::to_string(Poisons::_UsePoisonChance));
			Poisons::_Dosage = static_cast<int>(ini.GetLongValue("Poisons", "Dosage", Poisons::_Dosage));
			loginfo("[NUPSettings] {} {}", "Poisons:           Dosage", std::to_string(Poisons::_Dosage));

			// fortify potions
			FortifyPotions::_enableFortifyPotions = ini.GetBoolValue("FortifyPotions", "EnableFortifyPotionUsage", FortifyPotions::_enableFortifyPotions);
			loginfo("[NUPSettings] {} {}", "Fortify Potions:   EnableFortifyPotionUsage", std::to_string(FortifyPotions::_enableFortifyPotions));
			FortifyPotions::_DontUseWithWeaponsSheathed = ini.GetBoolValue("FortifyPotions", "DontUseWithWeaponsSheathed", FortifyPotions::_DontUseWithWeaponsSheathed);
			loginfo("[NUPSettings] {} {}", "Fortify Potions:   DontUseWithWeaponsSheathed", std::to_string(FortifyPotions::_DontUseWithWeaponsSheathed));
			FortifyPotions::_EnemyLevelScalePlayerLevelFortify = static_cast<float>(ini.GetDoubleValue("FortifyPotions", "EnemyLevelScalePlayerLevelFortify", FortifyPotions::_EnemyLevelScalePlayerLevelFortify));
			loginfo("[NUPSettings] {} {}", "Fortify Potions:   EnemyLevelScalePlayerLevelFortify", std::to_string(FortifyPotions::_EnemyLevelScalePlayerLevelFortify));
			FortifyPotions::_EnemyNumberThresholdFortify = ini.GetLongValue("FortifyPotions", "FightingNPCsNumberThresholdFortify", FortifyPotions::_EnemyNumberThresholdFortify);
			loginfo("[NUPSettings] {} {}", "Fortify Potions:   FightingNPCsNumberThresholdFortify", std::to_string(FortifyPotions::_EnemyNumberThresholdFortify));
			FortifyPotions::_UseFortifyPotionChance = static_cast<int>(ini.GetLongValue("FortifyPotions", "UseFortifyPotionChance", FortifyPotions::_UseFortifyPotionChance));
			loginfo("[NUPSettings] {} {}", "Fortify Potions:   UseFortifyPotionChance", std::to_string(FortifyPotions::_UseFortifyPotionChance));

			// food
			Food::_enableFood = ini.GetBoolValue("Food", "EnableFoodUsage", Food::_enableFood);
			loginfo("[NUPSettings] {} {}", "Food:              EnableFoodUsage", std::to_string(Food::_enableFood));
			Food::_AllowDetrimentalEffects = ini.GetBoolValue("Food", "AllowDetrimentalEffects", Food::_AllowDetrimentalEffects);
			loginfo("[NUPSettings] {} {}", "Food:              AllowDetrimentalEffects", std::to_string(Food::_AllowDetrimentalEffects));
			Food::_RestrictFoodToCombatStart = ini.GetBoolValue("Food", "OnlyAllowFoodAtCombatStart", Food::_RestrictFoodToCombatStart);
			loginfo("[NUPSettings] {} {}", "Food:              OnlyAllowFoodAtCombatStart", std::to_string(Food::_RestrictFoodToCombatStart));
			Food::_DisableFollowers = ini.GetBoolValue("Food", "DisableFollowers", Food::_DisableFollowers);
			loginfo("[NUPSettings] {} {}", "Food:              DisableFollowers", std::to_string(Food::_DisableFollowers));
			Food::_DontUseWithWeaponsSheathed = ini.GetBoolValue("Food", "DontUseWithWeaponsSheathed", Food::_DontUseWithWeaponsSheathed);
			loginfo("[NUPSettings] {} {}", "Food:              DontUseWithWeaponsSheathed", std::to_string(Food::_DontUseWithWeaponsSheathed));

			// player
			Player::_playerPotions = ini.GetBoolValue("Player", "EnablePlayerPotions", Player::_playerPotions);
			loginfo("[NUPSettings] {} {}", "Player:            EnablePlayerPotions", std::to_string(Player::_playerPotions));
			Player::_playerPoisons = ini.GetBoolValue("Player", "EnablePlayerPoisonUsage", Player::_playerPoisons);
			loginfo("[NUPSettings] {} {}", "Player:            EnablePlayerPoisonUsage", std::to_string(Player::_playerPoisons));
			Player::_playerFortifyPotions = ini.GetBoolValue("Player", "EnablePlayerFortifyPotionUsage", Player::_playerFortifyPotions);
			loginfo("[NUPSettings] {} {}", "Player:            EnablePlayerFortifyPotionUsage", std::to_string(Player::_playerFortifyPotions));
			Player::_playerFood = ini.GetBoolValue("Player", "EnablePlayerFoodUsage", Player::_playerFood);
			loginfo("[NUPSettings] {} {}", "Player:            EnablePlayerFoodUsage", std::to_string(Player::_playerFood));

			Player::_UseFavoritedItemsOnly = ini.GetBoolValue("Player", "UseFavoritedItemsOnly", Player::_UseFavoritedItemsOnly);
			loginfo("[NUPSettings] {} {}", "Player:            UseFavoritedItemsOnly", std::to_string(Player::_UseFavoritedItemsOnly));
			Player::_DontUseFavoritedItems = ini.GetBoolValue("Player", "DontUseFavoritedItems", Player::_DontUseFavoritedItems);
			loginfo("[NUPSettings] {} {}", "Player:            DontUseFavoritedItems", std::to_string(Player::_DontUseFavoritedItems));
			if (Player::_UseFavoritedItemsOnly && Player::_DontUseFavoritedItems)
				Player::_UseFavoritedItemsOnly = false;
			Player::_DontEatRawFood = ini.GetBoolValue("Player", "DontEatRawFood", Player::_DontEatRawFood);
			loginfo("[NUPSettings] {} {}", "Player:            DontEatRawFood", std::to_string(Player::_DontEatRawFood));
			Player::_DontDrinkAlcohol = ini.GetBoolValue("Player", "DontDrinkAlcohol", Player::_DontDrinkAlcohol);
			loginfo("[NUPSettings] {} {}", "Player:            DontDrinkAlcohol", std::to_string(Player::_DontDrinkAlcohol));

			// usage
			Usage::_DisableItemUsageWhileStaggered = ini.GetBoolValue("Usage", "DisableItemUsageWhileStaggered", Usage::_DisableItemUsageWhileStaggered);
			loginfo("[NUPSettings] {} {}", "usage:             DisableItemUsageWhileStaggered", std::to_string(Usage::_DisableItemUsageWhileStaggered));
			Usage::_DisableNonFollowerNPCs = ini.GetBoolValue("Usage", "DisableNonFollowerNPCs", Usage::_DisableNonFollowerNPCs);
			loginfo("[NUPSettings] {} {}", "usage:             DisableNonFollowerNPCs", std::to_string(Usage::_DisableNonFollowerNPCs));
			Usage::_DisableOutOfCombatProcessing = ini.GetBoolValue("Usage", "DisableOutOfCombatProcessing", Usage::_DisableOutOfCombatProcessing);
			loginfo("[NUPSettings] {} {}", "usage:             DisableOutOfCombatProcessing", std::to_string(Usage::_DisableOutOfCombatProcessing));
			Usage::_DisableItemUsageForExcludedNPCs = ini.GetBoolValue("Usage", "DisableItemUsageForExcludedNPCs", Usage::_DisableItemUsageForExcludedNPCs);
			loginfo("[NUPSettings] {} {}", "usage:             DisableItemUsageForExcludedNPCs", std::to_string(Usage::_DisableItemUsageForExcludedNPCs));
			Usage::_globalCooldown = ini.GetLongValue("Usage", "GlobalItemCooldown", Usage::_globalCooldown);
			loginfo("[NUPSettings] {} {}", "usage:             GlobalItemCooldown", std::to_string(Usage::_globalCooldown));

			// distribution

			Distr::_DistributePotions = ini.GetBoolValue("Distribution", "DistributePotions", Distr::_DistributePotions);
			loginfo("[NUPSettings] {} {}", "Distribution:      DistributePotions", std::to_string(Distr::_DistributePotions));
			Distr::_DistributePoisons = ini.GetBoolValue("Distribution", "DistributePoisons", Distr::_DistributePoisons);
			loginfo("[NUPSettings] {} {}", "Distribution:      DistributePoisons", std::to_string(Distr::_DistributePoisons));
			Distr::_DistributeFood = ini.GetBoolValue("Distribution", "DistributeFood", Distr::_DistributeFood);
			loginfo("[NUPSettings] {} {}", "Distribution:      DistributeFood", std::to_string(Distr::_DistributeFood));
			Distr::_DistributeFortifyPotions = ini.GetBoolValue("Distribution", "DistributeFortifyPotions", Distr::_DistributeFortifyPotions);
			loginfo("[NUPSettings] {} {}", "Distribution:      DistributeFortifyPotions", std::to_string(Distr::_DistributeFortifyPotions));
			Distr::_DistributeCustomItems = ini.GetBoolValue("Distribution", "DistributeCustomItems", Distr::_DistributeCustomItems);
			loginfo("[NUPSettings] {} {}", "Distribution:      DistributeCustomItems", std::to_string(Distr::_DistributeCustomItems));

			Distr::_LevelEasy = ini.GetLongValue("Distribution", "LevelEasy", Distr::_LevelEasy);
			loginfo("[NUPSettings] {} {}", "Distribution:      LevelEasy", std::to_string(Distr::_LevelEasy));
			Distr::_LevelNormal = ini.GetLongValue("Distribution", "LevelNormal", Distr::_LevelNormal);
			loginfo("[NUPSettings] {} {}", "Distribution:      LevelNormal", std::to_string(Distr::_LevelNormal));
			Distr::_LevelDifficult = ini.GetLongValue("Distribution", "LevelDifficult", Distr::_LevelDifficult);
			loginfo("[NUPSettings] {} {}", "Distribution:      LevelDifficult", std::to_string(Distr::_LevelDifficult));
			Distr::_LevelInsane = ini.GetLongValue("Distribution", "LevelInsane", Distr::_LevelInsane);
			loginfo("[NUPSettings] {} {}", "Distribution:      LevelInsane", std::to_string(Distr::_LevelInsane));

			Distr::_GameDifficultyScaling = ini.GetBoolValue("Distribution", "GameDifficultyScaling", Distr::_GameDifficultyScaling);
			loginfo("[NUPSettings] {} {}", "Distribution:      GameDifficultyScaling", std::to_string(Distr::_GameDifficultyScaling));

			Distr::_MaxMagnitudeWeak = ini.GetLongValue("Distribution", "MaxMagnitudeWeak", Distr::_MaxMagnitudeWeak);
			loginfo("[NUPSettings] {} {}", "Distribution:      MaxMagnitudeWeak", std::to_string(Distr::_MaxMagnitudeWeak));
			Distr::_MaxMagnitudeStandard = ini.GetLongValue("Distribution", "MaxMagnitudeStandard", Distr::_MaxMagnitudeStandard);
			loginfo("[NUPSettings] {} {}", "Distribution:      MaxMagnitudeStandard", std::to_string(Distr::_MaxMagnitudeStandard));
			Distr::_MaxMagnitudePotent = ini.GetLongValue("Distribution", "MaxMagnitudePotent", Distr::_MaxMagnitudePotent);
			loginfo("[NUPSettings] {} {}", "Distribution:      MaxMagnitudePotent", std::to_string(Distr::_MaxMagnitudePotent));

			Distr::_StyleScalingPrimary = (float)ini.GetDoubleValue("Distribution", "StyleScalingPrimary", Distr::_StyleScalingPrimary);
			loginfo("[NUPSettings] {} {}", "Distribution:      StyleScalingPrimary", std::to_string(Distr::_StyleScalingPrimary));
			Distr::_StyleScalingSecondary = (float)ini.GetDoubleValue("Distribution", "StyleScalingSecondary", Distr::_StyleScalingSecondary);
			loginfo("[NUPSettings] {} {}", "Distribution:      StyleScalingSecondary", std::to_string(Distr::_StyleScalingSecondary));

			// removal
			Removal::_RemoveItemsOnDeath = ini.GetBoolValue("Removal", "RemoveItemsOnDeath", Removal::_RemoveItemsOnDeath);
			loginfo("[NUPSettings] {} {}", "Removal:           RemoveItemsOnDeath", std::to_string(Removal::_RemoveItemsOnDeath));
			Removal::_ChanceToRemoveItem = ini.GetLongValue("Removal", "ChanceToRemoveItem", Removal::_ChanceToRemoveItem);
			loginfo("[NUPSettings] {} {}", "Removal:           ChanceToRemoveItem", std::to_string(Removal::_ChanceToRemoveItem));
			Removal::_MaxItemsLeft = ini.GetLongValue("Removal", "MaxItemsLeftAfterRemoval", Removal::_MaxItemsLeft);
			loginfo("[NUPSettings] {} {}", "Removal:           MaxItemsLeftAfterRemoval", std::to_string(Removal::_MaxItemsLeft));

			// whitelist mode
			Whitelist::EnabledItems = ini.GetBoolValue("Whitelist Mode", "EnableWhitelistItems", Whitelist::EnabledItems);
			loginfo("[NUPSettings] {} {}", "Whitelist Mode:    EnableWhitelistItems", std::to_string(Whitelist::EnabledItems));
			Whitelist::EnabledNPCs = ini.GetBoolValue("Whitelist Mode", "EnableWhitelistNPCs", Whitelist::EnabledNPCs);
			loginfo("[NUPSettings] {} {}", "Whitelist Mode:    EnableWhitelistNPCs", std::to_string(Whitelist::EnabledNPCs));

			// fixes
			Fixes::_ApplySkillBoostPerks = ini.GetBoolValue("Fixes", "ApplySkillBoostPerks", Fixes::_ApplySkillBoostPerks);
			loginfo("[NUPSettings] {} {}", "Fixes:             ApplySkillBoostPerks", std::to_string(Fixes::_ApplySkillBoostPerks));
			Fixes::_ForceFixPotionSounds = ini.GetBoolValue("Fixes", "ForceFixPotionSounds", Fixes::_ForceFixPotionSounds);
			loginfo("[NUPSettings] {} {}", "Fixes:             ForceFixPotionSounds", std::to_string(Fixes::_ForceFixPotionSounds));

			// system
			System::_cycletime = ini.GetLongValue("System", "CycleWaitTime", System::_cycletime);
			loginfo("[NUPSettings] {} {}", "System:            CycleWaitTime", std::to_string(System::_cycletime));

			// compatibility
			Compatibility::_DisableCreaturesWithoutRules = ini.GetBoolValue("Compatibility", "DisableCreaturesWithoutRules", Compatibility::_DisableCreaturesWithoutRules);
			loginfo("[NUPSettings] {} {}", "Compatibility:     DisableCreaturesWithoutRules", std::to_string(Compatibility::_DisableCreaturesWithoutRules));
			Compatibility::_CompatibilityMode = ini.GetBoolValue("Compatibility", "Compatibility", Compatibility::_CompatibilityMode);
			loginfo("[NUPSettings] {} {}", "Compatibility:     Compatibility", std::to_string(Compatibility::_CompatibilityMode));

			// compatibility animated poisons
			Compatibility::AnimatedPoisons::_Enable = ini.GetBoolValue("Compatibility: Animated Poisons", "EnableAnimatedPoisons", Compatibility::AnimatedPoisons::_Enable);
			loginfo("[NUPSettings] {} {}", "Compatibility:     EnableAnimatedPoisons", std::to_string(Compatibility::AnimatedPoisons::_Enable));
			Compatibility::AnimatedPoisons::_UsePoisonDosage = ini.GetBoolValue("Compatibility: Animated Poisons", "UseAnimatedPoisonsDosageSystem", Compatibility::AnimatedPoisons::_UsePoisonDosage);
			loginfo("[NUPSettings] {} {}", "Compatibility:     UseAnimatedPoisonsDosageSystem", std::to_string(Compatibility::AnimatedPoisons::_UsePoisonDosage));

			// compatibility animated potions
			Compatibility::AnimatedPotions::_Enable = ini.GetBoolValue("Compatibility: Animated Potions", "EnableAnimatedPotions", Compatibility::AnimatedPotions::_Enable);
			loginfo("[NUPSettings] {} {}", "Compatibility:     EnableAnimatedPotions", std::to_string(Compatibility::AnimatedPotions::_Enable));

			// debug
			Debug::EnableLog = ini.GetBoolValue("Debug", "EnableLogging", Debug::EnableLog);
			loginfo("[NUPSettings] {} {}", "Debug:             EnableLogging", std::to_string(Debug::EnableLog));
			Debug::EnableLoadLog = ini.GetBoolValue("Debug", "EnableLoadLogging", Debug::EnableLoadLog);
			loginfo("[NUPSettings] {} {}", "Debug:             EnableLoadLogging", std::to_string(Debug::EnableLoadLog));
			Debug::LogLevel = ini.GetLongValue("Debug", "LogLevel", Debug::LogLevel);
			loginfo("[NUPSettings] {} {}", "Debug:             LogLevel", std::to_string(Debug::LogLevel));
			Debug::EnableProfiling = ini.GetBoolValue("Debug", "EnableProfiling", Debug::EnableProfiling);
			loginfo("[NUPSettings] {} {}", "Debug:             EnableProfiling", std::to_string(Debug::EnableProfiling));
			Debug::ProfileLevel = ini.GetLongValue("Debug", "ProfileLevel", Debug::ProfileLevel);
			loginfo("[NUPSettings] {} {}", "Debug:             ProfileLevel", std::to_string(Debug::LogLevel));

			Debug::_CheckActorsWithoutRules = ini.GetBoolValue("Debug", "CheckActorWithoutRules", Debug::_CheckActorsWithoutRules);
			loginfo("[NUPSettings] {} {}", "Debug:             CheckActorWithoutRules", std::to_string(Debug::_CheckActorsWithoutRules));

			Debug::_CalculateCellRules = ini.GetBoolValue("Debug", "CalculateCellRules", Debug::_CalculateCellRules);
			loginfo("[NUPSettings] {} {}", "Debug:             CalculateCellRules", std::to_string(Debug::_CalculateCellRules));
			Debug::_Test = ini.GetBoolValue("Debug", "CalculateAllCellOnStartup", Debug::_Test);
			loginfo("[NUPSettings] {} {}", "Debug:             CalculateAllCellOnStartup", std::to_string(Debug::_Test));
			if (Debug::_CalculateCellRules && Debug::_Test == false) {
				std::ofstream out("Data\\SKSE\\Plugins\\NPCsUsePotions\\NPCsUsePotions_CellCalculation.csv", std::ofstream::out);
				out << "CellName;RuleApplied;PluginRef;ActorName;ActorBaseID;ReferenceID;RaceEditorID;RaceID;Cell;Factions\n";
			}

			Debug::_CompatibilityRemoveItemsBeforeDist = ini.GetBoolValue("Debug", "RemoveItemsBeforeDist", Debug::_CompatibilityRemoveItemsBeforeDist);
			loginfo("[NUPSettings] {} {}", "Debug:             RemoveItemsBeforeDist", std::to_string(Debug::_CompatibilityRemoveItemsBeforeDist));
			Debug::_CompatibilityRemoveItemsStartup = ini.GetBoolValue("Debug", "RemoveItemsStartup", Debug::_CompatibilityRemoveItemsStartup);
			loginfo("[NUPSettings] {} {}", "Debug:             RemoveItemsStartup", std::to_string(Debug::_CompatibilityRemoveItemsStartup));
			Debug::_CompatibilityRemoveItemsStartup_OnlyExcluded = ini.GetBoolValue("Debug", "RemoveItemsStartup_OnlyExcluded", Debug::_CompatibilityRemoveItemsStartup_OnlyExcluded);
			loginfo("[NUPSettings] {} {}", "Debug:             RemoveItemsStartup_OnlyExcluded", std::to_string(Debug::_CompatibilityRemoveItemsStartup_OnlyExcluded));
		}
	}
}

void Settings::Load()
{
	LOG_1("{}[Settings] [Load]");
	constexpr auto path = L"Data/SKSE/Plugins/DiseaseOverhaul.ini";

	CSimpleIniA ini;

	ini.SetUnicode();
	ini.LoadFile(path);

	// general

	Settings::System::_cycletime = ini.GetLongValue("General", "CycleWaitTime", Settings::System::_cycletime);
	loginfo("[SETTNGS] {} {}", "CycleWaitTime", std::to_string(Settings::System::_cycletime));
	System::_ticklength = (float)ini.GetDoubleValue("Game", "TickLength", 0.005);
	loginfo("[Settings] {} {}", "TickLength", std::to_string(System::_ticklength));

	// Debugging
	Debug::EnableLog = ini.GetBoolValue("Debug", "EnableLogging", Debug::EnableLog);
	Logging::EnableLog = Debug::EnableLog;
	loginfo("[SETTINGS] {} {}", "EnableLogging", std::to_string(Debug::EnableLog));
	Debug::EnableLoadLog = ini.GetBoolValue("Debug", "EnableLoadLogging", Debug::EnableLoadLog);
	Logging::EnableLoadLog = Debug::EnableLoadLog;
	loginfo("[SETTINGS] {} {}", "Debug:             EnableLoadLogging", std::to_string(Debug::EnableLoadLog));
	Debug::LogLevel = ini.GetLongValue("Debug", "LogLevel", 0);
	Logging::LogLevel = Debug::LogLevel;
	loginfo("[SETTINGS] {} {}", "LogLevel", std::to_string(Debug::LogLevel));
	Debug::EnableProfiling = ini.GetBoolValue("Debug", "EnableProfiling", Debug::EnableProfiling);
	Logging::EnableProfiling = Debug::EnableProfiling;
	loginfo("[SETTINGS] {} {}", "EnableProfiling", std::to_string(Debug::EnableProfiling));
	Debug::ProfileLevel = ini.GetLongValue("Debug", "ProfileLevel", 0);
	Logging::ProfileLevel = Debug::ProfileLevel;
	loginfo("[SETTINGS] {} {}", "ProfileLevel", std::to_string(Debug::LogLevel));

	// game settings
	Disease::_ignoreVampireBaseImmunity = ini.GetBoolValue("Game", "IgnoreVampireBaseImmunity", Disease::_ignoreVampireBaseImmunity);
	loginfo("[Settings] {} {}", "IgnoreVampireBaseImmunity", std::to_string(Disease::_ignoreVampireBaseImmunity));
	Disease::_ignoreWerewolfBaseImmunity = ini.GetBoolValue("Game", "IgnoreWerewolfBaseImmunity", Disease::_ignoreWerewolfBaseImmunity);
	loginfo("[Settings] {} {}", "IgnoreWerewolfBaseImmunity", std::to_string(Disease::_ignoreWerewolfBaseImmunity));
	Disease::_ignoreDiseaseResistance = ini.GetBoolValue("Game", "IgnoreDiseaseResistance", Disease::_ignoreDiseaseResistance);
	loginfo("[Settings] {} {}", "IgnoreDiseaseResistance", std::to_string(Disease::_ignoreDiseaseResistance));
	Disease::_particleRange = (float)ini.GetDoubleValue("Game", "ParticleInfectionRange", Disease::_particleRange);
	loginfo("[Settings] {} {}", "ParticleInfectionRange", std::to_string(Disease::_particleRange));
	Disease::_ignoreTimeAdvancementConstraint = (float)ini.GetDoubleValue("Game", "IgnoreTimeAdvancementConstraints", Disease::_ignoreTimeAdvancementConstraint);
	loginfo("[Settings] {} {}", "IgnoreTimeAdvancementConstraints", std::to_string(Disease::_ignoreTimeAdvancementConstraint));

	Save();

	auto datahandler = RE::TESDataHandler::GetSingleton();
	loginfo("[SETTINGS] checking for plugins");

	
	// Check for CACO
	{
		if (const uint32_t index = Utility::Mods::GetPluginIndex(Comp::CACO); index != 0x1) {
			loginfo("[SETTINGS] Complete Alchemy & Cooking Overhaul.esp is loaded, activating compatibility mode!");
			Compatibility::CACO::_CompatibilityCACO = true;
		}
	}
	// Check for Apothecary
	{
		if (const uint32_t index = Utility::Mods::GetPluginIndex(Comp::Apothecary); index != 0x1) {
			loginfo("[SETTINGS] Apothecary.esp is loaded, activating compatibility mode!");
			Compatibility::Apothecary::_CompatibilityApothecary = true;
		}
	}
	loginfo("[SETTINGS] checking for plugins end");
}

void Settings::Save()
{
	LOG_1("{}[Settings] [Save]");
	constexpr auto path = L"Data/SKSE/Plugins/DiseaseOverhaul.ini";

	CSimpleIniA ini;

	ini.SetUnicode();

	// general
	ini.SetLongValue("General", "CycleWaitTime", Settings::System::_cycletime,	"// Time between two periods in milliseconds.\n"
																				"// Set to smaller values to increase reactivity. Set to larger \n"
																				"// values to decrease performance impact.");
	ini.SetDoubleValue("General", "TickLength", Settings::System::_ticklength,	"// Time beween two calculation ticks in game days passed.");

	// debugging
	ini.SetBoolValue("Debug", "EnableLogging", Debug::EnableLog, "// Enables logging output. Use with care as logs may get very large.");
	ini.SetBoolValue("Debug", "EnableLoadLogging", Debug::EnableLoadLog,	"// Enables logging output for plugin load, use if you want to \n"
																			"// log rule issues");
	ini.SetLongValue("Debug", "LogLevel", Debug::LogLevel,
		"// 1 - layer 0 log entries, 2 - layer 1 log entries, 3 - layer 3 log entries, \n"
		"// 4 - layer 4 log entries. Affects which functions write log entries, \n"
		"// as well as what is written by those functions. ");
	ini.SetBoolValue("Debug", "EnableProfiling", Debug::EnableProfiling, "// Enables profiling output.");
	ini.SetLongValue("Debug", "ProfileLevel", Debug::ProfileLevel,
		"// 1 - only highest level functions write their executions times to \n"
		"// the profile log, 2 - lower level functions are written, 3 - lowest level \n"
		"// functions are written. Be aware that not all functions are supported \n"
		"// as Profiling costs execution time.");

	// game settings
	ini.SetBoolValue("Game", "IgnoreVampireBaseImmunity", Disease::_ignoreVampireBaseImmunity, "// Ignores the passive disease resistance of vampires");
	ini.SetBoolValue("Game", "IgnoreWerewolfBaseImmunity", Disease::_ignoreWerewolfBaseImmunity, "// Ignores the passive disease resistance of werewolfs");
	ini.SetBoolValue("Game", "IgnoreDiseaseResistance", Disease::_ignoreDiseaseResistance, "// Ignores any disease resistance");
	ini.SetDoubleValue("Game", "ParticleInfectionRange", Disease::_particleRange, "// Range at which npcs can infect each other via particle infection");
	ini.SetBoolValue("Game", "IgnoreTimeAdvancementConstraints", Disease::_ignoreTimeAdvancementConstraint, "// Ignores the minimum time needed for stage advancement");

	ini.SaveFile(path);
}
