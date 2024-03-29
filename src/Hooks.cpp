#include "Data.h"
#include "Events.h"
#include "Game.h"
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
		//AnimationEventHook::Install();
		Papyrus_FastTravelHook::InstallHook();
		FastTravelConfirmHook::InstallHook();
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

	void AnimationEventHook::AnimationEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* src)
	{
		logusage("[Hooks] [AnimationEvent]");
		//return _AnimationEvent(arg1, arg2, arg3, arg4);
	}

	void Papyrus_FastTravelHook::FastTravelBegin()
	{
		LOG_1("{}[Hooks] [FastTravelBegin] Begin Fast Travel");
		Game::SetFastTraveling(true);
		Events::Main::KillThreads();
	}

	bool FastTravelConfirmHook::FastTravelConfirm(uint64_t self, uint64_t menu)
	{
		LOG_1("{}[Hooks] [FastTravelConfirm] Begin FastTravel");
		Game::SetFastTraveling(true);
		Events::Main::KillThreads();
		return _FastTravelConfirm(self, menu);
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
