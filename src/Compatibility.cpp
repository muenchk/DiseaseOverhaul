#include <semaphore>

#include "Compatibility.h"
#include "Data.h"
#include "Game.h"
#include "Utility.h"


Compatibility* Compatibility::GetSingleton()
{
	static Compatibility singleton;
	return std::addressof(singleton);
}

std::binary_semaphore sem{ 1 };

void Compatibility::Load()
{
	// get lock to avoid deadlocks (should not occur, since the functions should not be called simultaneously
	sem.acquire();
	Data* data = Data::GetSingleton();
	RE::TESDataHandler* datahandler = RE::TESDataHandler::GetSingleton();

	// apothecary
	Apot_SH_AlcoholDrinkKeyword = RE::TESForm::LookupByID<RE::BGSKeyword>(0x01F3DD73);

	LOG1_1("{}[Compatibility] [Load] [Apot] {}", Utility::PrintForm(Apot_SH_AlcoholDrinkKeyword));

	if (Apot_SH_AlcoholDrinkKeyword &&
		Settings::Compatibility::Apothecary::_CompatibilityApothecary)
		_loadedApothecary = true;

	// caco
	CACO_VendorItemDrinkAlcohol = RE::TESForm::LookupByID<RE::BGSKeyword>(0x01AF101A);

	LOG1_1("{}[Compatibility] [Load] [CACO] {}", Utility::PrintForm(CACO_VendorItemDrinkAlcohol));

	if (CACO_VendorItemDrinkAlcohol &&
		Settings::Compatibility::CACO::_CompatibilityCACO) {
		_loadedCACO = true;
	}

	// global

	_globalCooldown = std::max((long)_globalCooldown, Settings::NUPSettings::Usage::_globalCooldown);

	LOG1_1("{}[Compatibility] [Load] GlobalCooldown set to {}ms", std::to_string(_globalCooldown));

	sem.release();
}

void Compatibility::Clear()
{
	// get lock to avoid deadlocks (should not occur, since the functions should not be called simultaneously
	sem.acquire();

	// apothecary
	_loadedApothecary = false;

	Apot_SH_AlcoholDrinkKeyword = nullptr;

	// caco
	_loadedCACO = false;

	CACO_VendorItemDrinkAlcohol = nullptr;

	// global
	_globalCooldown = 0;
	_disableParalyzedItems = false;

	sem.release();
}

void SaveGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
{
}

void LoadGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
{
	LOG_1("{}[Compatibility] [LoadGameCallback]");
	Compatibility::GetSingleton()->Load();
}

void RevertGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
{
	LOG_1("{}[Compatibility] [RevertGameCallback]");
	Compatibility::GetSingleton()->Clear();
}

void Compatibility::Register()
{
	Game::SaveLoad::GetSingleton()->RegisterForLoadCallback(0xFF000100, LoadGameCallback);
	LOG1_1("{}Registered {}", typeid(LoadGameCallback).name());
	Game::SaveLoad::GetSingleton()->RegisterForRevertCallback(0xFF000200, RevertGameCallback);
	LOG1_1("{}Registered {}", typeid(RevertGameCallback).name());
	Game::SaveLoad::GetSingleton()->RegisterForSaveCallback(0xFF000300, SaveGameCallback);
	LOG1_1("{}Registered {}", typeid(SaveGameCallback).name());
}
