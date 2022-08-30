#include"Events.h"
#include "Settings.h"
#include <string.h>
#include<chrono>
#include<thread>
#include <forward_list>
#include <semaphore>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <fstream>
#include <iostream>
#include "ActorManipulation.h"
#include <limits>
#include <filesystem>
#include <deque>
#include "ActorInfo.h"
#include <Game.h>
		
namespace Events
{
	using AlchemyEffect = Settings::AlchemyEffect;
#define Base(x) static_cast<uint64_t>(x)

	/// <summary>
	/// random number generator for processing probabilities
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	static std::mt19937 rand((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
	/// <summary>
	/// trims random numbers to 1 to 100
	/// </summary>
	static std::uniform_int_distribution<signed> rand100(1, 100);
	/// <summary>
	/// trims random numbers to 1 to 3
	/// </summary>
	static std::uniform_int_distribution<signed> rand3(1, 3);

#define Look(s) RE::TESForm::LookupByEditorID(s)

	/// <summary>
	/// determines whether events and functions are run
	/// </summary>
	static bool initialized = false;

	/// <summary>
	/// initializes importent variables, which need to be initialized every time a game is loaded
	/// </summary>
	void InitializeCompatibilityObjects()
	{ /*
		// now that the game was loaded we can try to initialize all our variables we conuldn't before
		if (!initialized) {
			// if we are in com mode, try to find the needed items. If we cannot find them, deactivate comp mode
			if (Settings::_CompatibilityPotionAnimatedFx) {
				RE::TESForm* tmp = RE::TESForm::LookupByEditorID(std::string_view{ Settings::Compatibility::PAF_NPCDrinkingCoolDownEffect_name });
				if (tmp)
					Settings::Compatibility::PAF_NPCDrinkingCoolDownEffect = tmp->As<RE::EffectSetting>();
				tmp = RE::TESForm::LookupByEditorID(std::string_view{ Settings::Compatibility::PAF_NPCDrinkingCoolDownSpell_name });
				if (tmp)
					Settings::Compatibility::PAF_NPCDrinkingCoolDownSpell = tmp->As<RE::SpellItem>();
				if (!(Settings::Compatibility::PAF_NPCDrinkingCoolDownEffect && Settings::Compatibility::PAF_NPCDrinkingCoolDownSpell)) {
					Settings::_CompatibilityPotionAnimatedFx = false;
					Settings::_CompatibilityPotionAnimatedFX_UseAnimations = false;
					logger::info("[INIT] Some Forms from PotionAnimatedfx.esp seem to be missing. Forcefully deactivated compatibility mode");
				}
			}
			// if compatibility mode for PotionnAnimatedFx is activated to use the animations, send events
			// with required variables to the papyrus scripts
			if (Settings::_CompatibilityPotionAnimatedFX_UseAnimations) {
				auto evs = SKSE::GetModCallbackEventSource();

				LOG_1("{}[LoadGameEvent] Setting variables for compatibility with PotionAnimatedfx.esp");
				SKSE::ModCallbackEvent* ev = nullptr;
				// send mod events to fill ALL the missing variables
				// since there are multiple plugins with the same name
				// and the same editor ids, but with different form
				// ids and we may only query FormIDs in papyrus

				LOG_4("{}[LoadGameEvent] Set 1");
				// PAF_DrinkSFX
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_DrinkSFX");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_DrinkSFX" })->As<RE::TESGlobal>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 2");
				// PAF_NPCDrinkingSlowVersion
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkingSlowVersion");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkingSlowVersion" })->As<RE::TESGlobal>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 3");
				// PAF_NPCFleeChance
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCFleeChance");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCFleeChance" })->As<RE::TESGlobal>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 6");
				// PAF_NPCDrinkingCoolDownSpell
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkingCoolDownSpell");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkingCoolDownSpell" })->As<RE::SpellItem>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 7");
				// PAF_NPCDrinkingInterruptDetectAbility
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkingInterruptDetectAbility");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkingInterruptDetectAbility" })->As<RE::SpellItem>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 8");
				// PAF_NPCDrinkFleeSpell
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkFleeSpell");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkFleeSpell" })->As<RE::SpellItem>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 9");
				// PAF_NPCDrinkingInterruptSpell
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkingInterruptSpell");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkingInterruptSpell" })->As<RE::SpellItem>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 10");
				// PAF_NPCDrinkingCoolDownEffect
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkingCoolDownEffect");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkingCoolDownEffect" })->As<RE::EffectSetting>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 11");
				// PAF_NPCDrinkingInterruptDetectEffect
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkingInterruptDetectEffect");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkingInterruptDetectEffect" })->As<RE::EffectSetting>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 12");
				// PAF_NPCDrinkFleeEffect
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkFleeEffect");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkFleeEffect" })->As<RE::EffectSetting>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 13");
				// PAF_NPCDrinkingInterruptEffect
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_NPCDrinkingInterruptEffect");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_NPCDrinkingInterruptEffect" })->As<RE::EffectSetting>();
					evs->SendEvent(ev);
				}
				LOG_4("{}[LoadGameEvent] Set 14");
				// PAF_MCMQuest
				{
					ev = new SKSE::ModCallbackEvent();
					ev->eventName = RE::BSFixedString("NDP_Comp_PAF_MCMQuest");
					ev->strArg = RE::BSFixedString("");
					ev->numArg = 0.0f;
					ev->sender = Look(std::string_view{ "PAF_MCMQuest" })->As<RE::TESQuest>();
					evs->SendEvent(ev);
				}

				// send control event to enable compatibility mode and check the delivered objects
				ev = new SKSE::ModCallbackEvent();
				ev->eventName = RE::BSFixedString("NPCsDrinkPotionCompPAFX");
				ev->strArg = RE::BSFixedString("");
				ev->numArg = 1.0f;
				ev->sender = nullptr;
				evs->SendEvent(ev);
				LOG_4("{}[LoadGameEvent] Sent controlevent");
			}
			initialized = true;
		}*/
	}

	/// <summary>
	/// Processes TESHitEvents
	/// </summary>
	/// <param name=""></param>
	/// <param name=""></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESHitEvent* /*a_event*/, RE::BSTEventSource<RE::TESHitEvent>*)
	{
		// currently unused
		return EventResult::kContinue;
	}

	// Actor, health cooldown, magicka cooldown, stamina cooldown, other cooldown, reg cooldown
	// static std::vector<std::tuple<RE::Actor*, int, int, int, int, int>> aclist{};

	/// <summary>
	/// list that holds currently handled actors
	/// </summary>
	static std::vector<ActorInfo*> aclist{};
	/// <summary>
	/// semaphore used to sync access to actor handling, to prevent list changes while operations are done
	/// </summary>
	static std::binary_semaphore sem(1);

	/// <summary>
	/// since the TESDeathEvent seems to be able to fire more than once for an actor we need to track the deaths
	/// </summary>
	static std::unordered_set<RE::FormID> deads;

	/// <summary>
	/// map that contains information about any npc that has entered combat during runtime
	/// </summary>
	static std::unordered_map<uint32_t, ActorInfo*> actorinfoMap;

	/// <summary>
	/// signals whether the player has died
	/// </summary>
	static bool playerdied = false;

	ActorInfo* FindActor(RE::Actor* actor)
	{
		ActorInfo* acinfo = nullptr;
		auto itr = actorinfoMap.find(actor->GetFormID());
		if (itr == actorinfoMap.end() || itr->second == nullptr) {
			acinfo = new ActorInfo(actor, 0, 0, 0, 0, 0);
			actorinfoMap.insert_or_assign(actor->GetFormID(), acinfo);
		} else
			acinfo = itr->second;
		return acinfo;
	}

#define CheckDeadEvent      \
	if (playerdied == true) \
		return EventResult::kContinue;

#define ReEvalPlayerDeath \
	if (RE::PlayerCharacter::GetSingleton()->IsDead() == false) { \
		playerdied = false; \
	} \
	else {                                                      \
		playerdied = true; \
	}

	/// <summary>
	/// EventHandler for TESDeathEvent
	/// removed unused potions and poisons from actor, to avoid economy instability
	/// only registered if itemremoval is activated in the settings
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		CheckDeadEvent;
		ReEvalPlayerDeath;
		InitializeCompatibilityObjects();
		auto actor = a_event->actorDying->As<RE::Actor>();
		if (actor->IsPlayerRef()) {
			playerdied = true;
		} else
		if (actor && actor != RE::PlayerCharacter::GetSingleton()) {
			// as with potion distribution, exlude excluded actors and potential followers
			if (!Settings::Distribution::ExcludedNPC(actor) && deads.contains(actor->GetFormID()) == false) {
				// create and insert new event
				CheckDeadEvent;
				ActorInfo* acinfo = FindActor(actor);
				deads.insert(actor->GetFormID());
				// distribute death items
				auto ditems = acinfo->FilterCustomConditionsDistrItems(acinfo->citems->death);
				// item, chance, num, cond1, cond2
				for (int i = 0; i < ditems.size(); i++) {
					// calc chances
					if (rand100(rand) <= std::get<1>(ditems[i])) {
						// distr item
						RE::ExtraDataList* extra = new RE::ExtraDataList();
						extra->SetOwner(actor);
						actor->AddObjectToContainer(std::get<0>(ditems[i]), extra, std::get<2>(ditems[i]), nullptr);
					}
				}
			}
		}

		return EventResult::kContinue;
	}

    /// <summary>
    /// handles TESCombatEvent
	/// registers the actor for tracking and handles giving them potions, poisons and food, beforehand.
	/// also makes then eat food before the fight.
    /// </summary>
    /// <param name="a_event">event parameters like the actor we need to handle</param>
    /// <param name=""></param>
    /// <returns></returns>
    EventResult EventHandler::ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*)
	{
		CheckDeadEvent;
		ReEvalPlayerDeath;

		return EventResult::kContinue;
    }

	/// <summary>
	/// [true] if the actorhandler is running, [false] if the thread died
	/// </summary>
	static bool actorhandlerrunning = false;
	/// <summary>
	/// thread running the CheckActors function
	/// </summary>
	static std::thread* actorhandler = nullptr;


#define CheckDeadCheckHandlerLoop \
	if (playerdied) { \
		break; \
	}


	/// <summary>
	/// EventHandler for TESLoadGameEvent. Loads main thread
	/// </summary>
	/// <param name="">unused</param>
	/// <param name="">unused</param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*)
	{
		LOG_1("{}[LoadGameEvent]");
		// if we canceled the main thread, reset that
		initialized = false;
		// reset regsitered actors, since we skip unregister events after player has died
		sem.acquire();
		for (int i = 0; i < aclist.size(); i++) {
			delete aclist[i];
		}
		aclist.clear();
		sem.release();
		// set player to alive
		ReEvalPlayerDeath;
		// reset actor lastDistrTime until it is saved in save game
		sem.acquire();
		auto itr = actorinfoMap.begin();
		while (itr != actorinfoMap.end()) {
			itr->second->lastDistrTime = 0.0f;
		}
		sem.release();
		// reset the list of actors that died
		deads.clear();

		return EventResult::kContinue;
	}

	void LoadGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[LoadGameCallback]");
	}

	void SaveGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[SaveGameCallback]");
	}
	
	/// <summary>
	/// EventHandler for Debug purposes. It calculates the distribution rules for all npcs in the cell
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		CheckDeadEvent;
		ReEvalPlayerDeath;

		return EventResult::kContinue;
	}

    /// <summary>
    /// returns singleton to the EventHandler
    /// </summary>
    /// <returns></returns>
    EventHandler* EventHandler::GetSingleton()
    {
        static EventHandler singleton;
        return std::addressof(singleton);
    }

    /// <summary>
    /// Registers us for all Events we want to receive
    /// </summary>
	void EventHandler::Register()
	{
		auto scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		scriptEventSourceHolder->GetEventSource<RE::TESHitEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESHitEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESLoadGameEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESLoadGameEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESEquipEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESEquipEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESDeathEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESDeathEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESActorLocationChangeEvent>()->AddEventSink(Eventhandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESDeathEvent).name());
		Game::SaveLoad::GetSingleton()->RegisterForLoadCallback(0xFF000001, LoadGameCallback);
		Game::SaveLoad::GetSingleton()->RegisterForLoadCallback(0xFF000002, SaveGameCallback);
	}

	/// <summary>
	/// Registers all EventHandlers, if we would have multiple
	/// </summary>
	void RegisterAllEventHandlers()
	{
		EventHandler::Register();
		LOG_1("{}Registered all event handlers"sv);
	}

	/// <summary>
	/// sets the main threads to stop on the next iteration
	/// </summary>
	void DisableThreads()
	{
	}

	/// <summary>
	/// Resets information about actors
	/// </summary>
	void ResetActorInfoMap()
	{
		sem.acquire();
		auto itr = actorinfoMap.begin();
		while (itr != actorinfoMap.end()) {
			itr->second->_boss = false;
			itr->second->citems->Reset();
		}
		sem.release();
	}

}
