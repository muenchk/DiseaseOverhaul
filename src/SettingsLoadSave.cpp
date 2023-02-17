#include "Settings.h"

void Settings::LoadNUP()
{
	LOG_1("{}[Settings] [LoadNUP]");
	constexpr auto path = L"Data/SKSE/Plugins/NPCsUsePotions.ini";

	bool Ultimateoptions = false;

	CSimpleIniA ini;

	ini.SetUnicode();
	ini.LoadFile(path);

	// Features
	NUPSettings::_featMagickaRestoration = ini.GetValue("Features", "EnableMagickaRestoration") ? ini.GetBoolValue("Features", "EnableMagickaRestoration") : true;
	loginfo("[NUPSETTINGS] {} {}", "EnableMagickaRestoration", std::to_string(NUPSettings::_featMagickaRestoration));
	NUPSettings::_featStaminaRestoration = ini.GetValue("Features", "EnableStaminaRestoration") ? ini.GetBoolValue("Features", "EnableStaminaRestoration") : true;
	loginfo("[NUPSETTINGS] {} {}", "EnableStaminaRestoration", std::to_string(NUPSettings::_featStaminaRestoration));
	NUPSettings::_featHealthRestoration = ini.GetValue("Features", "EnableHealthRestoration") ? ini.GetBoolValue("Features", "EnableHealthRestoration") : true;
	loginfo("[NUPSETTINGS] {} {}", "EnableHealthRestoration", std::to_string(NUPSettings::_featHealthRestoration));
	NUPSettings::_featUsePoisons = ini.GetValue("Features", "EnablePoisonUsage") ? ini.GetBoolValue("Features", "EnablePoisonUsage") : true;
	loginfo("[NUPSETTINGS] {} {}", "EnablePoisonUsage", std::to_string(NUPSettings::_featUsePoisons));
	NUPSettings::_featUseFortifyPotions = ini.GetValue("Features", "EnableFortifyPotionUsage") ? ini.GetBoolValue("Features", "EnableFortifyPotionUsage") : true;
	loginfo("[NUPSETTINGS] {} {}", "EnableFortifyPotionUsage", std::to_string(NUPSettings::_featUseFortifyPotions));
	NUPSettings::_featUseFood = ini.GetValue("Features", "EnableFoodUsage") ? ini.GetBoolValue("Features", "EnableFoodUsage") : true;
	loginfo("[NUPSETTINGS] {} {}", "EnableFoodUsage", std::to_string(NUPSettings::_featUseFood));

	NUPSettings::_playerRestorationEnabled = ini.GetValue("Features", "EnablePlayerRestoration") ? ini.GetBoolValue("Features", "EnablePlayerRestoration") : false;
	loginfo("[NUPSETTINGS] {} {}", "EnablePlayerRestoration", std::to_string(NUPSettings::_playerRestorationEnabled));
	NUPSettings::_playerUsePoisons = ini.GetValue("Features", "EnablePlayerPoisonUsage") ? ini.GetBoolValue("Features", "EnablePlayerPoisonUsage") : false;
	loginfo("[NUPSETTINGS] {} {}", "EnablePlayerPoisonUsage", std::to_string(NUPSettings::_playerUsePoisons));
	NUPSettings::_playerUseFortifyPotions = ini.GetValue("Features", "EnablePlayerFortifyPotionUsage") ? ini.GetBoolValue("Features", "EnablePlayerFortifyPotionUsage") : false;
	loginfo("[NUPSETTINGS] {} {}", "EnablePlayerFortifyPotionUsage", std::to_string(NUPSettings::_playerUseFortifyPotions));

	NUPSettings::_featDistributePotions = ini.GetValue("Features", "DistributePotions") ? ini.GetBoolValue("Features", "DistributePotions") : true;
	loginfo("[NUPSETTINGS] {} {}", "DistributePotions", std::to_string(NUPSettings::_featDistributePotions));
	NUPSettings::_featDistributePoisons = ini.GetValue("Features", "DistributePoisons") ? ini.GetBoolValue("Features", "DistributePoisons") : true;
	loginfo("[NUPSETTINGS] {} {}", "DistributePoisons", std::to_string(NUPSettings::_featDistributePoisons));
	NUPSettings::_featDistributeFood = ini.GetValue("Features", "DistributeFood") ? ini.GetBoolValue("Features", "DistributeFood") : true;
	loginfo("[NUPSETTINGS] {} {}", "DistributeFood", std::to_string(NUPSettings::_featDistributeFood));
	NUPSettings::_featDistributeFortifyPotions = ini.GetValue("Features", "DistributeFortifyPotions") ? ini.GetBoolValue("Features", "DistributeFortifyPotions") : true;
	loginfo("[NUPSETTINGS] {} {}", "DistributeFortifyPotions", std::to_string(NUPSettings::_featDistributeFortifyPotions));

	NUPSettings::_featRemoveItemsOnDeath = ini.GetValue("Features", "RemoveItemsOnDeath") ? ini.GetBoolValue("Features", "RemoveItemsOnDeath") : true;
	loginfo("[NUPSETTINGS] {} {}", "RemoveItemsOnDeath", std::to_string(NUPSettings::_featRemoveItemsOnDeath));

	NUPSettings::_featDisableItemUsageWhileStaggered = ini.GetValue("Features", "DisableItemUsageWhileStaggered") ? ini.GetBoolValue("Features", "DisableItemUsageWhileStaggered") : false;
	loginfo("[NUPSETTINGS] {} {}", "DisableItemUsageWhileStaggered", std::to_string(NUPSettings::_featDisableItemUsageWhileStaggered));

	// distribution
	NUPSettings::_LevelEasy = ini.GetValue("Distribution", "LevelEasy") ? ini.GetLongValue("Distribution", "LevelEasy") : NUPSettings::_LevelEasy;
	loginfo("[NUPSETTINGS] {} {}", "LevelEasy", std::to_string(NUPSettings::_LevelEasy));
	NUPSettings::_LevelNormal = ini.GetValue("Distribution", "LevelNormal") ? ini.GetLongValue("Distribution", "LevelNormal") : NUPSettings::_LevelNormal;
	loginfo("[NUPSETTINGS] {} {}", "LevelNormal", std::to_string(NUPSettings::_LevelNormal));
	NUPSettings::_LevelDifficult = ini.GetValue("Distribution", "LevelDifficult") ? ini.GetLongValue("Distribution", "LevelDifficult") : NUPSettings::_LevelDifficult;
	loginfo("[NUPSETTINGS] {} {}", "LevelDifficult", std::to_string(NUPSettings::_LevelDifficult));
	NUPSettings::_LevelInsane = ini.GetValue("Distribution", "LevelInsane") ? ini.GetLongValue("Distribution", "LevelInsane") : NUPSettings::_LevelInsane;
	loginfo("[NUPSETTINGS] {} {}", "LevelInsane", std::to_string(NUPSettings::_LevelInsane));

	NUPSettings::_GameDifficultyScaling = ini.GetValue("Distribution", "GameDifficultyScaling") ? ini.GetBoolValue("Distribution", "GameDifficultyScaling") : false;
	loginfo("[NUPSETTINGS] {} {}", "GameDifficultyScaling", std::to_string(NUPSettings::_GameDifficultyScaling));

	NUPSettings::_MaxMagnitudeWeak = ini.GetValue("Distribution", "MaxMagnitudeWeak") ? ini.GetLongValue("Distribution", "MaxMagnitudeWeak") : NUPSettings::_MaxMagnitudeWeak;
	loginfo("[NUPSETTINGS] {} {}", "MaxMagnitudeWeak", std::to_string(NUPSettings::_MaxMagnitudeWeak));
	NUPSettings::_MaxMagnitudeStandard = ini.GetValue("Distribution", "MaxMagnitudeStandard") ? ini.GetLongValue("Distribution", "MaxMagnitudeStandard") : NUPSettings::_MaxMagnitudeStandard;
	loginfo("[NUPSETTINGS] {} {}", "MaxMagnitudeStandard", std::to_string(NUPSettings::_MaxMagnitudeStandard));
	NUPSettings::_MaxMagnitudePotent = ini.GetValue("Distribution", "MaxMagnitudePotent") ? ini.GetLongValue("Distribution", "MaxMagnitudePotent") : NUPSettings::_MaxMagnitudePotent;
	loginfo("[NUPSETTINGS] {} {}", "MaxMagnitudePotent", std::to_string(NUPSettings::_MaxMagnitudePotent));

	// Restoration Thresholds
	NUPSettings::_healthThreshold = ini.GetValue("Restoration", "HealthThresholdPercent") ? static_cast<float>(ini.GetDoubleValue("Restoration", "HealthThresholdPercent")) : NUPSettings::_healthThreshold;
	NUPSettings::_healthThreshold = ini.GetValue("Restoration", "HealthThresholdLowerPercent") ? static_cast<float>(ini.GetDoubleValue("Restoration", "HealthThresholdLowerPercent")) : NUPSettings::_healthThreshold;
	if (NUPSettings::_healthThreshold > 0.95f)
		NUPSettings::_healthThreshold = 0.95f;
	loginfo("[NUPSETTINGS] {} {}", "HealthThresholdPercent", std::to_string(NUPSettings::_healthThreshold));
	NUPSettings::_magickaThreshold = ini.GetValue("Restoration", "MagickaThresholdPercent") ? static_cast<float>(ini.GetDoubleValue("Restoration", "MagickaThresholdPercent")) : NUPSettings::_magickaThreshold;
	NUPSettings::_magickaThreshold = ini.GetValue("Restoration", "MagickaThresholdLowerPercent") ? static_cast<float>(ini.GetDoubleValue("Restoration", "MagickaThresholdLowerPercent")) : NUPSettings::_magickaThreshold;
	if (NUPSettings::_magickaThreshold > 0.95f)
		NUPSettings::_magickaThreshold = 0.95f;
	loginfo("[NUPSETTINGS] {} {}", "MagickaThresholdPercent", std::to_string(NUPSettings::_magickaThreshold));
	NUPSettings::_staminaThreshold = ini.GetValue("Restoration", "StaminaThresholdPercent") ? static_cast<float>(ini.GetDoubleValue("Restoration", "StaminaThresholdPercent")) : NUPSettings::_staminaThreshold;
	NUPSettings::_staminaThreshold = ini.GetValue("Restoration", "StaminaThresholdLowerPercent") ? static_cast<float>(ini.GetDoubleValue("Restoration", "StaminaThresholdLowerPercent")) : NUPSettings::_staminaThreshold;
	if (NUPSettings::_staminaThreshold > 0.95f)
		NUPSettings::_staminaThreshold = 0.95f;
	loginfo("[NUPSETTINGS] {} {}", "StaminaThresholdPercent", std::to_string(NUPSettings::_staminaThreshold));
	NUPSettings::_UsePotionChance = ini.GetValue("Restoration", "UsePotionChance") ? static_cast<int>(ini.GetLongValue("Restoration", "UsePotionChance")) : NUPSettings::_UsePotionChance;
	loginfo("[NUPSETTINGS] {} {}", "UsePotionChance", std::to_string(NUPSettings::_UsePotionChance));

	// Poisonusage options
	NUPSettings::_EnemyLevelScalePlayerLevel = ini.GetValue("Poisons", "EnemyLevelScalePlayerLevel") ? static_cast<float>(ini.GetDoubleValue("Poisons", "EnemyLevelScalePlayerLevel")) : NUPSettings::_EnemyLevelScalePlayerLevel;
	loginfo("[NUPSETTINGS] {} {}", "EnemyLevelScalePlayerLevel", std::to_string(NUPSettings::_EnemyLevelScalePlayerLevel));
	NUPSettings::_EnemyNumberThreshold = ini.GetValue("Poisons", "FightingNPCsNumberThreshold") ? ini.GetLongValue("Poisons", "FightingNPCsNumberThreshold") : NUPSettings::_EnemyNumberThreshold;
	loginfo("[NUPSETTINGS] {} {}", "FightingNPCsNumberThreshold", std::to_string(NUPSettings::_EnemyNumberThreshold));
	NUPSettings::_UsePoisonChance = ini.GetValue("Poisons", "UsePoisonChance") ? static_cast<int>(ini.GetLongValue("Poisons", "UsePoisonChance")) : NUPSettings::_UsePoisonChance;
	loginfo("[NUPSETTINGS] {} {}", "UsePoisonChance", std::to_string(NUPSettings::_UsePoisonChance));

	// fortify options
	NUPSettings::_EnemyLevelScalePlayerLevelFortify = ini.GetValue("Fortify", "EnemyLevelScalePlayerLevelFortify") ? static_cast<float>(ini.GetDoubleValue("Fortify", "EnemyLevelScalePlayerLevelFortify")) : NUPSettings::_EnemyLevelScalePlayerLevelFortify;
	loginfo("[NUPSETTINGS] {} {}", "EnemyLevelScalePlayerLevelFortify", std::to_string(NUPSettings::_EnemyLevelScalePlayerLevelFortify));
	NUPSettings::_EnemyNumberThresholdFortify = ini.GetValue("Fortify", "FightingNPCsNumberThresholdFortify") ? ini.GetLongValue("Fortify", "FightingNPCsNumberThresholdFortify") : NUPSettings::_EnemyNumberThresholdFortify;
	loginfo("[NUPSETTINGS] {} {}", "FightingNPCsNumberThresholdFortify", std::to_string(NUPSettings::_EnemyNumberThresholdFortify));
	NUPSettings::_UseFortifyPotionChance = ini.GetValue("Fortify", "UseFortifyPotionChance") ? static_cast<int>(ini.GetLongValue("Fortify", "UseFortifyPotionChance")) : NUPSettings::_UseFortifyPotionChance;
	loginfo("[NUPSETTINGS] {} {}", "UseFortifyPotionChance", std::to_string(NUPSettings::_UseFortifyPotionChance));

	// removal options
	NUPSettings::_ChanceToRemoveItem = ini.GetValue("Removal", "ChanceToRemoveItem") ? ini.GetLongValue("Removal", "ChanceToRemoveItem") : NUPSettings::_ChanceToRemoveItem;
	loginfo("[NUPSETTINGS] {} {}", "ChanceToRemoveItem", std::to_string(NUPSettings::_ChanceToRemoveItem));
	NUPSettings::_MaxItemsLeft = ini.GetValue("Removal", "MaxItemsLeftAfterRemoval") ? ini.GetLongValue("Removal", "MaxItemsLeftAfterRemoval") : NUPSettings::_MaxItemsLeft;
	loginfo("[NUPSETTINGS] {} {}", "MaxItemsLeftAfterRemoval", std::to_string(NUPSettings::_MaxItemsLeft));

	// general
	NUPSettings::_maxPotionsPerCycle = ini.GetValue("General", "MaxPotionsPerCycle") ? ini.GetLongValue("General", "MaxPotionsPerCycle", 1) : 1;
	loginfo("[NUPSETTINGS] {} {}", "MaxPotionsPerCycle", std::to_string(NUPSettings::_maxPotionsPerCycle));
	NUPSettings::_cycletime = ini.GetValue("General", "CycleWaitTime") ? ini.GetLongValue("General", "CycleWaitTime", 1000) : 1000;
	loginfo("[NUPSETTINGS] {} {}", "CycleWaitTime", std::to_string(NUPSettings::_cycletime));
	NUPSettings::_DisableEquipSounds = ini.GetValue("General", "DisableEquipSounds") ? ini.GetBoolValue("General", "DisableEquipSounds", false) : false;
	loginfo("[NUPSETTINGS] {} {}", "DisableEquipSounds", std::to_string(NUPSettings::_DisableEquipSounds));

	// Debugging
	NUPSettings::EnableLog = ini.GetValue("Debug", "EnableLogging") ? ini.GetBoolValue("Debug", "EnableLogging") : false;
	loginfo("[NUPSETTINGS] {} {}", "EnableLogging", std::to_string(NUPSettings::EnableLog));
	NUPSettings::LogLevel = ini.GetValue("Debug", "LogLevel") ? ini.GetLongValue("Debug", "LogLevel") : 0;
	loginfo("[NUPSETTINGS] {} {}", "LogLevel", std::to_string(NUPSettings::LogLevel));
	NUPSettings::EnableProfiling = ini.GetValue("Debug", "EnableProfiling") ? ini.GetBoolValue("Debug", "EnableProfiling") : false;
	loginfo("[NUPSETTINGS] {} {}", "EnableProfiling", std::to_string(NUPSettings::EnableProfiling));
	NUPSettings::ProfileLevel = ini.GetValue("Debug", "ProfileLevel") ? ini.GetLongValue("Debug", "ProfileLevel") : 0;
	loginfo("[NUPSETTINGS] {} {}", "ProfileLevel", std::to_string(NUPSettings::LogLevel));

	NUPSettings::_CheckActorsWithoutRules = ini.GetBoolValue("Debug", "CheckActorWithoutRules", false);
	loginfo("[NUPSETTINGS] {} {}", "CheckActorWithoutRules", std::to_string(NUPSettings::_CheckActorsWithoutRules));

	NUPSettings::_CalculateCellRules = ini.GetBoolValue("Debug", "CalculateCellRules", false);
	loginfo("[NUPSETTINGS] {} {}", "CalculateCellRules", std::to_string(NUPSettings::_CalculateCellRules));
	NUPSettings::_Test = ini.GetBoolValue("Debug", "CalculateAllCellOnStartup", false);
	loginfo("[NUPSETTINGS] {} {}", "CalculateAllCellOnStartup", std::to_string(NUPSettings::_Test));
}

void Settings::LoadAlchExt()
{
	LOG_1("{}[Settings] [LoadAlchExt]");
	constexpr auto path = L"Data/SKSE/Plugins/AlchemyExtension.ini";

	CSimpleIniA ini;

	ini.SetUnicode();
	ini.LoadFile(path);

	// general

	Settings::AlchExtSettings::CycleTime = ini.GetLongValue("General", "CycleWaitTime", Settings::AlchExtSettings::CycleTime);
	loginfo("[SETTNGS] {} {}", "CycleWaitTime", std::to_string(Settings::AlchExtSettings::CycleTime));
	AlchExtSettings::TickLength = (float)ini.GetDoubleValue("Game", "TickLength", 0.005);
	loginfo("[Settings] {} {}", "TickLength", std::to_string(AlchExtSettings::TickLength));

	// Debugging
	AlchExtSettings::EnableLog = ini.GetValue("Debug", "EnableLogging") ? ini.GetBoolValue("Debug", "EnableLogging") : false;
	Logging::EnableLog = AlchExtSettings::EnableLog;
	loginfo("[SETTINGS] {} {}", "EnableLogging", std::to_string(AlchExtSettings::EnableLog));
	AlchExtSettings::LogLevel = ini.GetValue("Debug", "LogLevel") ? ini.GetLongValue("Debug", "LogLevel") : 0;
	Logging::LogLevel = AlchExtSettings::LogLevel;
	loginfo("[SETTINGS] {} {}", "LogLevel", std::to_string(AlchExtSettings::LogLevel));
	AlchExtSettings::EnableProfiling = ini.GetValue("Debug", "EnableProfiling") ? ini.GetBoolValue("Debug", "EnableProfiling") : false;
	Logging::EnableProfiling = AlchExtSettings::EnableProfiling;
	loginfo("[SETTINGS] {} {}", "EnableProfiling", std::to_string(AlchExtSettings::EnableProfiling));
	AlchExtSettings::ProfileLevel = ini.GetValue("Debug", "ProfileLevel") ? ini.GetLongValue("Debug", "ProfileLevel") : 0;
	Logging::ProfileLevel = AlchExtSettings::ProfileLevel;
	loginfo("[SETTINGS] {} {}", "ProfileLevel", std::to_string(AlchExtSettings::LogLevel));

	// game settings
	AlchExtSettings::_ignoreVampireBaseImmunity = ini.GetBoolValue("Game", "IgnoreVampireBaseImmunity", false);
	loginfo("[Settings] {} {}", "IgnoreVampireBaseImmunity", std::to_string(AlchExtSettings::_ignoreVampireBaseImmunity));
	AlchExtSettings::_ignoreWerewolfBaseImmunity = ini.GetBoolValue("Game", "IgnoreWerewolfBaseImmunity", false);
	loginfo("[Settings] {} {}", "IgnoreWerewolfBaseImmunity", std::to_string(AlchExtSettings::_ignoreWerewolfBaseImmunity));
	AlchExtSettings::_ignoreDiseaseResistance = ini.GetBoolValue("Game", "IgnoreDiseaseResistance", false);
	loginfo("[Settings] {} {}", "IgnoreDiseaseResistance", std::to_string(AlchExtSettings::_ignoreDiseaseResistance));
	AlchExtSettings::_particleRange = (float)ini.GetDoubleValue("Game", "ParticleInfectionRange", AlchExtSettings::_particleRange);
	loginfo("[Settings] {} {}", "ParticleInfectionRange", std::to_string(AlchExtSettings::_particleRange));
	AlchExtSettings::_ignoreTimeAdvancementConstraint = (float)ini.GetDoubleValue("Game", "IgnoreTimeAdvancementConstraints", false);
	loginfo("[Settings] {} {}", "IgnoreTimeAdvancementConstraints", std::to_string(AlchExtSettings::_ignoreTimeAdvancementConstraint));

	SaveAlchExt();

	auto datahandler = RE::TESDataHandler::GetSingleton();
	loginfo("[SETTINGS] checking for plugins");

	// Check for CACO
	{
		if (const RE::TESFile* plugin = datahandler->LookupModByName(std::string_view{ "Complete Alchemy & Cooking Overhaul.esp" }); plugin) {
			loginfo("[SETTINGS] Complete Alchemy & Cooking Overhaul.esp is loaded, activating compatibility mode!");
			AlchExtSettings::_CompatibilityCACO = true;
		}
	}
	// Check for Apothecary
	{
		if (const RE::TESFile* plugin = datahandler->LookupModByName(std::string_view{ "Apothecary.esp" }); plugin) {
			loginfo("[SETTINGS] Apothecary.esp is loaded, activating compatibility mode!");
			AlchExtSettings::_CompatibilityApothecary = true;
		}
	}
	loginfo("[SETTINGS] checking for plugins end");
}

void Settings::SaveAlchExt()
{
	LOG_1("{}[Settings] [SaveAlchExt]");
	constexpr auto path = L"Data/SKSE/Plugins/AlchemyExtension.ini";

	CSimpleIniA ini;

	ini.SetUnicode();

	// general
	ini.SetLongValue("General", "CycleWaitTime", Settings::AlchExtSettings::CycleTime, ";Time between two calculation cycles in milliseconds");
	ini.SetDoubleValue("General", "TickLength", Settings::AlchExtSettings::TickLength, ";Time beween two calculation ticks in game days passed.");

	// debugging
	ini.SetBoolValue("Debug", "EnableLogging", AlchExtSettings::EnableLog, ";Enables logging output. Use with care as log may get very large");
	ini.SetLongValue("Debug", "LogLevel", AlchExtSettings::LogLevel, ";1 - layer 0 log entries, 2 - layer 1 log entries, 3 - layer 3 log entries, 4 - layer 4 log entries. Affects which functions write log entries, as well as what is written by those functions. ");
	ini.SetBoolValue("Debug", "EnableProfiling", AlchExtSettings::EnableProfiling, ";Enables profiling output.");
	ini.SetLongValue("Debug", "ProfileLevel", AlchExtSettings::ProfileLevel, ";1 - only highest level functions write their executions times to the log, 2 - lower level functions are written, 3 - lowest level functions are written. Be aware that not all functions are supported as Profiling costs execution time.");

	// game settings
	ini.SetBoolValue("Game", "IgnoreVampireBaseImmunity", AlchExtSettings::_ignoreVampireBaseImmunity, ";Ignores the passive disease resistance of vampires");
	ini.SetBoolValue("Game", "IgnoreWerewolfBaseImmunity", AlchExtSettings::_ignoreWerewolfBaseImmunity, ";Ignores the passive disease resistance of werewolfs");
	ini.SetBoolValue("Game", "IgnoreDiseaseResistance", AlchExtSettings::_ignoreDiseaseResistance, ";Ignores any disease resistance");
	ini.SetDoubleValue("Game", "ParticleInfectionRange", AlchExtSettings::_particleRange, ";Range at which npcs can infect each other via particle infection");
	ini.SetBoolValue("Game", "IgnoreTimeAdvancementConstraints", AlchExtSettings::_ignoreTimeAdvancementConstraint, ";Ignores the minimum time needed for stage advancement");

	ini.SaveFile(path);
}
