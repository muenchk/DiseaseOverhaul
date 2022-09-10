#include <string.h>
#include <chrono>
#include <thread>
#include <forward_list>
#include <semaphore>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <fstream>
#include <iostream>
#include <limits>
#include <filesystem>
#include <deque>

#include "ActorInfo.h"
#include "Game.h"
#include "Utility.h"
#include "Events.h"
#include "Settings.h"
		
namespace Events
{
	using AlchemyEffect = AlchemyEffect;
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
	/// enables all active functions
	/// </summary>
	static bool enableProcessing = false;
#define EvalProcessing()   \
	if (!enableProcessing) \
		return;
#define EvalProcessingEvent() \
	if (!enableProcessing)    \
		return EventResult::kContinue;
	bool CanProcess()
	{
		return enableProcessing;
	}

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
			initialized = true;
		}*/
	}

	ActorInfo* FindActor(RE::Actor* actor)
	{
		ActorInfo* acinfo = nullptr;
		auto itr = actorinfoMap.find(actor->GetFormID());
		if (itr == actorinfoMap.end()) {
			acinfo = new ActorInfo(actor, 0, 0, 0, 0, 0);
			actorinfoMap.insert_or_assign(actor->GetFormID(), acinfo);
		} else if (itr->second == nullptr || itr->second->actor == nullptr || itr->second->actor->GetFormID() == 0 || itr->second->actor->GetFormID() != actor->GetFormID()) {
			// either delete acinfo, deleted actor, actor fid 0 or acinfo belongs to wrong actor
			actorinfoMap.erase(actor->GetFormID());
			acinfo = new ActorInfo(actor, 0, 0, 0, 0, 0);
			actorinfoMap.insert_or_assign(actor->GetFormID(), acinfo);
		} else {
			acinfo = itr->second;
			if (acinfo->citems == nullptr)
				acinfo->citems = new ActorInfo::CustomItems();
		}
		return acinfo;
	}

#define ReEvalPlayerDeath                                         \
	if (RE::PlayerCharacter::GetSingleton()->IsDead() == false) { \
		playerdied = false;                                       \
	} else {                                                      \
		playerdied = true;                                        \
	}

	/// <summary>
	/// Processes TESHitEvents
	/// </summary>
	/// <param name=""></param>
	/// <param name=""></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESHitEvent* /*a_event*/, RE::BSTEventSource<RE::TESHitEvent>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [TESHitEvent]");
		// currently unused
		return EventResult::kContinue;
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
		EvalProcessingEvent();
		LOG_1("{}[Events] [DeathEvent]");
		ReEvalPlayerDeath;
		InitializeCompatibilityObjects();
		auto actor = a_event->actorDying->As<RE::Actor>();
		if (actor->IsPlayerRef()) {
			playerdied = true;
		} else
		if (actor && actor != RE::PlayerCharacter::GetSingleton()) {
			// as with potion distribution, exlude excluded actors and potential followers
			if (!Distribution::ExcludedNPC(actor) && deads.contains(actor->GetFormID()) == false) {
				// create and insert new event
				EvalProcessingEvent();
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
	/// EventHandler for Harvested Items
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESHarvestedEvent::ItemHarvested* a_event, RE::BSTEventSource<RE::TESHarvestedEvent::ItemHarvested>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [HarvestEvent]");
		ReEvalPlayerDeath;
		// check that all event fields are valid
		if (a_event && a_event->produceItem && a_event->harvester) {
			LOG2_1("{}[Events] [HarvestEvent] harvested: {} {}", Utility::GetHex(a_event->produceItem->GetFormID()), a_event->produceItem->GetName());
			auto itr = Settings::AlchExtRuntime::harvestMap()->find(a_event->produceItem->GetFormID());
			if (itr != Settings::AlchExtRuntime::harvestMap()->end()) {
				auto vec = itr->second;
				auto iter = vec.begin();
				while (iter != vec.end()) {
					if (*iter == nullptr) {
						// error item not valid, clean up vector
						iter = vec.erase(iter);
						Settings::AlchExtRuntime::harvestMap()->insert_or_assign(a_event->produceItem->GetFormID(), vec);
					} else if ((*iter)->object == nullptr) {
						delete *iter;
						iter = vec.erase(iter);
						Settings::AlchExtRuntime::harvestMap()->insert_or_assign(a_event->produceItem->GetFormID(), vec);
					} else {
						for (int x = 0; x < (*iter)->num; x++) {
							if (rand100(rand) < (*iter)->chance) {
								RE::ExtraDataList* extra = new RE::ExtraDataList();
								extra->SetOwner(a_event->harvester);
								a_event->harvester->AddObjectToContainer((*iter)->object, extra, 1, nullptr);
							}
						}
					}
					iter++;
				}
			}
		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Debug purposes. It calculates the distribution rules for all npcs in the cell
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* /*a_event*/, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [EquipEvent]");
		ReEvalPlayerDeath;

		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Actors being attached / detached
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [TESCellAttachDetachEvent]");
		ReEvalPlayerDeath;

		if (a_event && a_event->reference) {
			RE::Actor* actor = a_event->reference->As<RE::Actor>();
			if (actor) {
			}
		}

		return EventResult::kContinue;
	}


	/// <summary>
	/// EventHandler for TESLoadGameEvent. Loads main thread
	/// </summary>
	/// <param name="">unused</param>
	/// <param name="">unused</param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*)
	{
		LOG_1("{}[Events] [LoadGameEvent]");
		// if we canceled the main thread, reset that
		initialized = false;
		// set player to alive
		ReEvalPlayerDeath;
		// reset the list of actors that died
		deads.clear();
		
		enableProcessing = true;

		return EventResult::kContinue;
	}

	void LoadGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [LoadGameCallback]");
	}

	void SaveGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [SaveGameCallback]");
	}

	void RevertGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [Events] [RevertGameCallback]");
		enableProcessing = false;
		auto itr = actorinfoMap.begin();
		while (itr != actorinfoMap.end()) {
			if (itr->second != nullptr)
				try {
					delete itr->second;
				} catch (std::exception&) {}
			itr++;
		}
		// reset actor information
		actorinfoMap.clear();
		// reset actor processing list
		aclist.clear();
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
		scriptEventSourceHolder->GetEventSource<RE::TESCellAttachDetachEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESCellAttachDetachEvent).name());
		RE::TESHarvestedEvent::GetEventSource()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESHarvestedEvent::ItemHarvested).name());
		Game::SaveLoad::GetSingleton()->RegisterForLoadCallback(0xFF000001, LoadGameCallback);
		Game::SaveLoad::GetSingleton()->RegisterForSaveCallback(0xFF000002, SaveGameCallback);
		Game::SaveLoad::GetSingleton()->RegisterForRevertCallback(0xFF000003, RevertGameCallback);
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
			if (itr->second)
				itr->second->_boss = false;
			itr->second->citems->Reset();
		}
		sem.release();
	}

}
