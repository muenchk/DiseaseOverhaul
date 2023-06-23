#pragma once
#include "Settings.h"
#include <unordered_map>

class Compatibility
{
public:
	// apothecary
	static inline std::string Apothecary = "Apothecary.esp";
	RE::BGSKeyword* Apot_SH_AlcoholDrinkKeyword = nullptr;

	// caco
	static inline std::string CACO = "Complete Alchemy & Cooking Overhaul.esp";
	RE::BGSKeyword* CACO_VendorItemDrinkAlcohol = nullptr;

	// general section
private:
	/// <summary>
	/// Whether all objects for Apothecary have been found
	/// </summary>
	bool _loadedApothecary = false;
	/// <summary>
	/// Whether all objects for Complete Alchemy and Cooking Overhaul have been found
	/// </summary>
	bool _loadedCACO = false;

	/// <summary>
	/// Global cooldown applied
	/// </summary>
	int _globalCooldown = 0;

	/// <summary>
	/// whether item usage while paralyzed is disabled
	/// </summary>
	bool _disableParalyzedItems = false;

public:
	/// <summary>
	/// Returns a static Compatibility object
	/// </summary>
	/// <returns></returns>
	static Compatibility* GetSingleton();

	/// <summary>
	/// Loads all game items for the mods, and verifies that compatibility has been established
	/// </summary>
	void Load();

	/// <summary>
	/// Clears all loaded data from memory
	/// </summary>
	void Clear();

	bool CanApplyPoisonToLeftHand()
	{
		// all anmiation mods must be false, then we may use left hand
		return true;
	}

	/// <summary>
	/// returns whether compatibility for apothecary is enabled
	/// </summary>
	/// <returns></returns>
	bool LoadedApothecary()
	{
		return Settings::Compatibility::Apothecary::_CompatibilityApothecary && _loadedApothecary;
	}

	/// <summary>
	/// returns whether compatibility for caco is enabled
	/// </summary>
	/// <returns></returns>
	bool LoadedCACO()
	{
		return Settings::Compatibility::CACO::_CompatibilityCACO && _loadedCACO;
	}

	/// <summary>
	/// returns whether item usage should be disabled while an actor is paralyzed, considering the settings and the loaded plugins
	/// </summary>
	/// <returns></returns>
	bool DisableItemUsageWhileParalyzed()
	{
		return Settings::NUPSettings::Usage::_DisableItemUsageWhileStaggered || _disableParalyzedItems;
	}

	/// <summary>
	/// Returns the global cooldown for item usage
	/// </summary>
	/// <returns></returns>
	int GetGlobalCooldown()
	{
		return _globalCooldown;
	}

	/// <summary>
	/// Registers Game Callbacks
	/// </summary>
	static void Register();
};
