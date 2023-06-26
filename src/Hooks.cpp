#include "Data.h"
#include "Hooks.h"
#include "Logging.h"
#include "Settings.h"
#include "UtilityAlch.h"

Data* data = nullptr;

namespace Hooks
{

	void InstallHooks()
	{
		data = Data::GetSingleton();
		//GetNameHook::InstallHook();
		if (Settings::Infection::_ShowInfectionStatus)
			PowerOfThree::Install();
	}

	RE::BSFixedString GetNameHook::GetName(RE::TESForm* form)
	{
		logusage("[Hooks] [GetName] {}", Utility::PrintFormNonDebug(form));

		if (auto actor = form->As<RE::Actor>(); actor != nullptr) {
			logusage("[Hooks] [GetName] 1");
			if (data->FindActorExisting(actor)->IsInfected()) {
				logusage("[Hooks] [GetName] 2");
				return RE::BSFixedString((std::string(_GetName(form)) + " - Infected").c_str());
			}
		}
		logusage("[Hooks] [GetName] 3");
		return _GetName(form);
	}
}

namespace Hooks_Funcs
{
	const char* GetName(RE::TESObjectREFR* obj, const char* origName)
	{
		if (auto actor = obj->As<RE::Actor>(); actor != nullptr) {
			if (auto acinfo = data->FindActorExisting(actor); acinfo->IsInfected()) {
				acinfo->_altName = origName + std::string(" - Infected");
				return acinfo->_altName.c_str();
			}
		}
		return origName;
	}
}
