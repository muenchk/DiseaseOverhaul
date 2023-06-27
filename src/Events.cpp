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
#include "UtilityAlch.h"
#include "Events.h"
#include "Settings.h"
#include "Data.h"
#include "Random.h"
#include "Stats.h"
#include "WorldspaceController.h"
		
namespace Events
{
#define Base(x) static_cast<uint64_t>(x)

#define Look(s) RE::TESForm::LookupByEditorID(s)

#pragma region Data

#define EvalProcessing()   \
	if (!enableProcessing) \
		return;
#define GetProcessing() \
	enableProcessing
#define WaitProcessing()      \
	while (!enableProcessing) \
		std::this_thread::sleep_for(100ms);
#define EvalProcessingEvent() \
	if (!Main::CanProcess())  \
		return EventResult::kContinue;

#define CheckDeadEvent                       \
	LOG1_1("{}[PlayerDead] {}", playerdied); \
	if (playerdied == true) {                \
		return EventResult::kContinue;       \
	}

#define ReEvalPlayerDeath                                         \
	if (RE::PlayerCharacter::GetSingleton()->IsDead() == false) { \
		playerdied = false;                                       \
	}

#define CheckDeadCheckHandlerLoop \
	if (playerdied) {             \
		break;                    \
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
		auto begin = std::chrono::steady_clock::now();
		Main::InitializeCompatibilityObjects();
		RE::Actor* actor = nullptr;
		if (a_event == nullptr || a_event->actorDying == nullptr) {
			LOG_4("{}[Events] [TESDeathEvent] Died due to invalid event");
			goto TESDeathEventEnd;
		}
		actor = a_event->actorDying->As<RE::Actor>();
		if (!Utility::ValidateActor(actor)) {
			LOG_4("{}[Events] [TESDeathEvent] Died due to actor validation fail");
			goto TESDeathEventEnd;
		}
		if (Utility::ValidateActor(actor)) {
			if (actor->IsPlayerRef()) {
				Main::PlayerDied(true);
			} else if (actor && actor != RE::PlayerCharacter::GetSingleton()) {
				// if not already dead, do stuff
				if (Main::IsDeadEventFired(actor) == false) {
					Main::SetDead(actor);
					EvalProcessingEvent();
					// invalidate actor
					std::shared_ptr<ActorInfo> acinfo = Main::data->FindActorExisting(actor);
					bool excluded = Distribution::ExcludedNPC(acinfo);
					acinfo->SetDead();
					// all npcs must be unregistered, even if distribution oes not apply to them
					Main::UnregisterNPC(actor);
					// as with potion distribution, exlude excluded actors and potential followers
					if (!excluded) {
						// distribute death items, independently of whether the npc is excluded
						auto ditems = acinfo->FilterCustomConditionsDistrItems(acinfo->citems.death);
						// item, chance, num, cond1, cond2
						for (int i = 0; i < ditems.size(); i++) {
							// calc chances
							if (Random::rand100(Random::rand) <= ditems[i]->chance) {
								// distr item
								actor->AddObjectToContainer(ditems[i]->object, nullptr, ditems[i]->num, nullptr);
							}
						}
						acinfo->SetInvalid();
						// delete actor from data
						Main::data->DeleteActor(actor->GetFormID());
					}
				}
			}
		}
TESDeathEventEnd:
		PROF1_1("{}[Events] [TESDeathEvent] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));

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
							if (Random::rand100(Random::rand) < (*iter)->chance) {
								a_event->harvester->AddObjectToContainer((*iter)->object, nullptr, 1, nullptr);
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
		//LOG_1("{}[Events] [TESCellAttachDetachEvent]");
		Main::PlayerDied((bool)(RE::PlayerCharacter::GetSingleton()->GetActorRuntimeData().boolBits & RE::Actor::BOOL_BITS::kDead));
		//auto begin = std::chrono::steady_clock::now();

		if (a_event && a_event->reference) {
			RE::Actor* actor = a_event->reference->As<RE::Actor>();
			if (Utility::ValidateActor(actor) && !Main::IsDead(actor) && !actor->IsPlayerRef()) {
				if (a_event->attached) {
					if (Distribution::ExcludedNPCFromHandling(actor) == false)
						Main::RegisterNPC(actor);
				} else {
					Main::UnregisterNPC(actor);
				}
			}
		}

		return EventResult::kContinue;
	}
	/// <summary>
	/// Handles an item being removed from a container
	/// </summary>
	/// <param name="container">The container the item was removed from</param>
	/// <param name="baseObj">The base object that has been removed</param>
	/// <param name="count">The number of items that have been removed</param>
	/// <param name="destinationContainer">The container the items have been moved to</param>
	/// <param name="a_event">The event information</param>
	void EventHandler::OnItemRemoved(RE::TESObjectREFR* container, RE::TESBoundObject* baseObj, int /*count*/, RE::TESObjectREFR* /*destinationContainer*/, const RE::TESContainerChangedEvent* /*a_event*/)
	{
		LOG2_1("{}[Events] [OnItemRemovedEvent] {} removed from {}", Utility::PrintForm(baseObj), Utility::PrintForm(container));
		RE::Actor* actor = container->As<RE::Actor>();
		if (actor) {
			// handle event for an actor
			//std::shared_ptr<ActorInfo> acinfo = data->FindActor(actor);
			/* if (comp->LoadedAnimatedPoisons()) {
				// handle removed poison
				RE::AlchemyItem* alch = baseObj->As<RE::AlchemyItem>();
				if (alch && alch->IsPoison()) {
					LOG_1("{}[Events] [OnItemRemovedEvent] AnimatedPoison animation");

					//ACM::AnimatedPoison_ApplyPoison(acinfo, alch);

					//std::string AnimationEventString = "poisondamagehealth02";
					//acinfo->actor->NotifyAnimationGraph(AnimationEventString);
						
					//RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> point(nullptr);
					//a_vm->DispatchStaticCall("Debug", "SendAnimationEvent", SKSE::Impl::VMArg(actor, RE::BSFixedString("poisondamagehealth02")), point);
					//RE::MakeFunctionArguments(actor, RE::BSFixedString("poisondamagehealth02"));
				}
			}
			*/
		}

		// handle event for generic reference

		return;
	}

	/// <summary>
	/// Handles an item being added to a container
	/// </summary>
	/// <param name="container">The container the item is added to</param>
	/// <param name="baseObj">The base object that has been added</param>
	/// <param name="count">The number of items added</param>
	/// <param name="sourceContainer">The container the item was in before</param>
	/// <param name="a_event">The event information</param>
	void EventHandler::OnItemAdded(RE::TESObjectREFR* container, RE::TESBoundObject* baseObj, int /*count*/, RE::TESObjectREFR* /*sourceContainer*/, const RE::TESContainerChangedEvent* /*a_event*/)
	{
		LOG2_1("{}[Events] [OnItemAddedEvent] {} added to {}", Utility::PrintForm(baseObj), Utility::PrintForm(container));
		RE::Actor* actor = container->As<RE::Actor>();
		if (actor) {
			// handle event for an actor
			//std::shared_ptr<ActorInfo> acinfo = data->FindActor(actor);
		}

		// handle event for generic objects
		return;
	}

	/// <summary>
	/// EventHandler for catching deleted forms / actors
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*)
	{
		// very important event. Allows to catch actors and other stuff that gets deleted, without dying, which could cause CTDs otherwise
		Stats::Events_TESFormDeleteEvent++;
		if (a_event && a_event->formID != 0) {
			Main::data->DeleteActor(a_event->formID);
			Main::UnregisterNPC(a_event->formID);
			Main::data->DeleteFormCustom(a_event->formID);
		}
		return EventResult::kContinue;
	}

	EventResult EventHandler::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*)
	{
		// very important event. Allows to catch actors and other stuff that gets deleted, without dying, which could cause CTDs otherwise
		
		if (auto actor = a_event->actor.get().get(); actor != nullptr)
		{
			if (actor->IsPlayerRef())
				World::GetSingleton()->PlayerChangeCell(a_event->cellID);
		}
		return EventResult::kContinue;
	}


	EventResult EventHandler::ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>* /*a_eventSource*/)
	{
		// this event handles all object transfers between containers in the game
		// this can be deived into multiple base events: OnItemRemoved and OnItemAdded
		Stats::Events_TESContainerChangedEvent++;
		EvalProcessingEvent();

		if (a_event && a_event->baseObj != 0 && a_event->itemCount != 0) {
			RE::TESObjectREFR* oldCont = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_event->oldContainer);
			RE::TESObjectREFR* newCont = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_event->newContainer);
			RE::TESBoundObject* baseObj = RE::TESForm::LookupByID<RE::TESBoundObject>(a_event->baseObj);
			if (baseObj && oldCont) {
				OnItemRemoved(oldCont, baseObj, a_event->itemCount, newCont, a_event);
			}
			if (baseObj && newCont) {
				OnItemAdded(newCont, baseObj, a_event->itemCount, oldCont, a_event);
			}
		}

		return EventResult::kContinue;
	}
	
	RE::Actor* target = nullptr;
	RE::TESObjectREFR* targetobj = nullptr;

	EventResult EventHandler::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* /*a_eventSource*/)
	{
		using EventType = RE::INPUT_EVENT_TYPE;
		using DeviceType = RE::INPUT_DEVICE;

		if (!a_event) {
			return EventResult::kContinue;
		}

		for (auto event = *a_event; event; event = event->next) {
			if (event->eventType != EventType::kButton) {
				continue;
			}

			auto& userEvent = event->QUserEvent();
			auto userEvents = RE::UserEvents::GetSingleton();

			auto button = static_cast<RE::ButtonEvent*>(event);
			if (button->IsDown()) {
				auto key = button->idCode;
				switch (button->device.get()) {
				case DeviceType::kMouse:
					key += 256;
					break;
				case DeviceType::kKeyboard:
					key += 0;
					break;
				default:
					continue;
				}

				// handle key here
				auto console = RE::ConsoleLog::GetSingleton();

				if (key == 0x52) { // numpad 0
					LOG_3("{}[Events] [InputEvent] registered target selection event");
					// lock target 
					targetobj = RE::CrosshairPickData::GetSingleton()->targetActor.get().get();
					if (targetobj) {
						target = targetobj->As<RE::Actor>();
						if (target) {
							console->Print(std::string("AlchExt: Changed target actor").c_str());
							return EventResult::kContinue;
						}
					}
					target = RE::PlayerCharacter::GetSingleton();
					console->Print(std::string("AlchExt: Changed target to player").c_str());
				} else if (key == 0x4f) {  // numpad 1
					LOG_3("{}[Events] [InputEvent] registered stage increase event");
					// increase stage target
					if (target) {
						std::shared_ptr<ActorInfo> acinfo = Main::data->FindActor(target);
						acinfo->ForceIncreaseStage(Diseases::kAshWoeBlight);
						console->Print(std::string("AlchExt: Increased target stage").c_str());
					} else
						console->Print(std::string("AlchExt: Missing target").c_str());
				} else if (key == 0x50) {  // numpad 2
					LOG_3("{}[Events] [InputEvent] registered stage decrease event");
					// decrease stage target
					if (target) {
						std::shared_ptr<ActorInfo> acinfo = Main::data->FindActor(target);
						acinfo->ForceDecreaseStage(Diseases::kAshWoeBlight);
						console->Print(std::string("AlchExt: Decreased target stage").c_str());
					} else
						console->Print(std::string("AlchExt: Missing target").c_str());
				} else if (key == 0x51) {  // numpad 3
					LOG_3("{}[Events] [InputEvent] registered print information event");
					// print disease information
					if (target) {
						std::shared_ptr<ActorInfo> acinfo = Main::data->FindActor(target);
						console->Print((std::string("AlchExt: Printing target information: \t") + target->GetName()).c_str());
						console->Print((std::string("Vampire: \t\t") + std::to_string(acinfo->IsVampire())).c_str());
						console->Print((std::string("Automaton: \t\t") + std::to_string(acinfo->IsAutomaton())).c_str());
						console->Print((std::string("Werewolf: \t\t") + std::to_string(acinfo->IsWerewolf())).c_str());
						console->Print((std::string("Printing disease information: \t")).c_str());
						for (auto dis : acinfo->diseases) {
							if (dis != nullptr) {
								console->Print((std::string("\tDisease:\t\t\t") + Main::data->GetDisease(dis->disease)->GetName()).c_str());
								console->Print((std::string("\t\tstatus:\t\t") + UtilityAlch::ToString(dis->status)).c_str());
								console->Print((std::string("\t\tstage:\t\t") + std::to_string(dis->stage)).c_str());
								console->Print((std::string("\t\tadvPoints:\t") + std::to_string(dis->advPoints)).c_str());
								console->Print((std::string("\t\tearladv:\t\t") + std::to_string(dis->earliestAdvancement)).c_str());
								console->Print((std::string("\t\timmuneuntil:\t") + std::to_string(dis->immuneUntil)).c_str());
								console->Print((std::string("\t\tpermanentMods:\t") + std::to_string(dis->permanentModifiers)).c_str());
								console->Print((std::string("\t\tpermanentPoints:\t") + std::to_string(dis->permanentModifiersPoints)).c_str());
							}
						}
					} else
						console->Print(std::string("AlchExt: Missing target").c_str());
				} else if (key == 0x4b) {  // numpad 4
					LOG_3("{}[Events] [InputEvent] registered inc adv points event");
					if (target) {
						std::shared_ptr<ActorInfo> acinfo = Main::data->FindActor(target);
						bool kill = acinfo->ProgressDisease(Diseases::kAshWoeBlight, 500);
						console->Print(std::string("AlchExt: Decreased target stage").c_str());
					} else
						console->Print(std::string("AlchExt: Missing target").c_str());
					// increase adv points by 500
				} else if (key == 0x4c) {  // numpad 5
					LOG_3("{}[Events] [InputEvent] registered print stats event");
					// print stats
					console->Print((std::string("MainHandler_Run:                 ") + std::to_string(Stats::MainHandler_Run)).c_str());
					console->Print((std::string("MainHandler_ActorsHandled:       ") + std::to_string(Stats::MainHandler_ActorsHandled)).c_str());
					console->Print((std::string("MainHandler_TimeTake:            ") + std::to_string(Stats::MainHandler_TimeTaken) + "us").c_str());
					console->Print((std::string("DiseaseStats_ProgressDisease:    ") + std::to_string(Stats::DiseaseStats_ProgressDisease)).c_str());
					console->Print((std::string("Average Particle Calc Time:      ") + std::to_string(UtilityAlch::Sum(Stats::MainHandler_Particles_Times) / Stats::MainHandler_Particles_Times.size())).c_str());
				} else if (key == 0x4d) {  // numpad 6
					LOG_3("{}[Events] [InputEvent] registered reset stats event");
					console->Print(std::string("Reset Stats").c_str());
					Stats::DiseaseStats_ProgressDisease = 0;
					Stats::MainHandler_ActorsHandled = 0;
					Stats::MainHandler_ActorsHandledTotal = 0;
					Stats::MainHandler_Run = 0;
					Stats::MainHandler_TimeTaken = 0;
					Stats::MainHandler_Particles_Times.clear();
				} else if (key == 0x47) {  // numpad 7
					LOG_3("{}[Events] [InputEvent] registered switch particle handler event");
					console->Print(std::string("Switched particle handler, deprecated").c_str());
					Main::particlehandling = !Main::particlehandling;
				} else if (key == 0x48) {  // numpad 8
				} else if (key == 0x49) {  // numpad 9
				}
			}
		}
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
		scriptEventSourceHolder->GetEventSource<RE::TESEquipEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESEquipEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESDeathEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESDeathEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESCellAttachDetachEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESCellAttachDetachEvent).name());
		RE::TESHarvestedEvent::GetEventSource()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESHarvestedEvent::ItemHarvested).name());
		RE::BSInputDeviceManager::GetSingleton()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::InputEvent).name());
		Game::SaveLoad::GetSingleton()->RegisterForLoadCallback(0xFF000001, Main::LoadGameCallback);
		Game::SaveLoad::GetSingleton()->RegisterForSaveCallback(0xFF000002, Main::SaveGameCallback);
		Game::SaveLoad::GetSingleton()->RegisterForRevertCallback(0xFF000003, Main::RevertGameCallback);
		Main::data = Data::GetSingleton();
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

}
