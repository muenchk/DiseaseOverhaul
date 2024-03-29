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

#include <Windows.h>
#include <realtimeapiset.h>

#include "ActorInfo.h"
#include "Game.h"
#include "UtilityAlch.h"
#include "Events.h"
#include "Settings.h"
#include "Data.h"
#include "Papyrus.h"
#include "Papyrus/PapyrusVampirism.h"
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


	enum class HitType
	{
		kNone,
		kMelee,
		kH2H,
		kRanged
	};
						
#define TimeMultSeconds 10000000
#define TimeMultMillis 10000

#define Time100Millis 1000000

	EventResult EventHandler::ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
	{
		if (a_event && a_event->holder) {
			bool actionPhysical = false;
			bool actionMagical = false;
			bool actionVoice = false;
			float scale = 1;

			if (a_event->tag == "weaponSwing" || a_event->tag == "weaponLeftSwing") {
				actionPhysical = true;
			} else if (a_event->tag == "blockStartOut") {
			} else if (a_event->tag == "PowerAttackStop") {
				// power attacks also fire "weaponSwing" so only add a little bit as penalty and not the full powerattack value
				actionPhysical = true;
				scale = 0.5;
			} else if (a_event->tag == "JumpUp") {
				actionPhysical = true;
			} else if (a_event->tag == "bowDraw") {
				actionPhysical = true;
			} else if (a_event->tag == "BeginCastVoice") {
				actionVoice = true;
			} else if (a_event->tag == "MRh_SpellFire_Event" || a_event->tag == "MLh_SpellFire_Event") {
				actionMagical = true;
			}

			// if we don't have anything to do, no need to look up the npc
			if (actionPhysical || actionMagical || actionVoice) {
				LOG1_1("{}[Events] [BSAnimationGraphEvent] {}", a_event->tag);
				if (std::shared_ptr<ActorInfo> acinfo = Main::data->FindActorExisting(a_event->holder->As<RE::Actor>()); acinfo && acinfo->IsValid()) {
					for (int i = 0; i < Diseases::kMaxValue; i++) {
						Diseases::Disease disval = static_cast<Diseases::Disease>(i);
						std::shared_ptr<Disease> dis = Main::data->GetDisease(disval);
						if (!dis) {
							//LOG_1("{}[Events] [HandleActors] skip disease");
							continue;
						}
						std::shared_ptr<DiseaseStage> stage = nullptr;
						auto dinfo = acinfo->FindDisease(disval);
						if (!dinfo)
							stage = dis->_stageInfection;
						else
							stage = dis->_stages[dinfo->stage];
						float scale = 1;
						scale = scale * (1 - acinfo->IgnoresDisease());

						if (scale != 0) {
							if (actionPhysical) {
								if (stage->_spreading[Spreading::kActionPhysical].second > 0)
									acinfo->AddDiseasePoints(disval, scale * stage->_spreading[Spreading::kActionPhysical].second);
							} else if (actionMagical) {
								if (stage->_spreading[Spreading::kActionMagical].second > 0)
									acinfo->AddDiseasePoints(disval, scale * stage->_spreading[Spreading::kActionMagical].second);
							} else if (actionVoice) {
								if (stage->_spreading[Spreading::kActionVoice].second > 0)
									acinfo->AddDiseasePoints(disval, scale * stage->_spreading[Spreading::kActionVoice].second);
							}
						}
					}
					acinfo->ProgressAllDiseases();
				}
			}
		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// Processes TESHitEvents
	/// </summary>
	/// <param name=""></param>
	/// <param name=""></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*)
	{
		EvalProcessingEvent();
		// if hit is blocked, skip
		if (a_event->flags & RE::TESHitEvent::Flag::kHitBlocked)
			return EventResult::kContinue;

		unsigned long long currtime = 0;
		QueryUnbiasedInterruptTime(&currtime);

		// currently unused
		if (a_event->target.get() != nullptr && a_event->cause.get() != nullptr) {
			if (RE::Actor* target = a_event->target->As<RE::Actor>(); target != nullptr) {
				if (RE::Actor* aggressor = a_event->cause->As<RE::Actor>(); aggressor != nullptr) {
					LOG_1("{}[Events] [TESHitEvent]");
					auto actar = Main::data->FindActor(target);
					// if less than interval has passed since the last hitevent on the actor return
					if (currtime - actar->GetHitCooldown() < Time100Millis)
						return EventResult::kContinue;
					actar->SetHitCooldown(currtime);
					auto acagg = Main::data->FindActor(aggressor);

					// skip if both actors aren't infected
					if (!actar->IsInfected() && !acagg->IsInfected())
						return EventResult::kContinue;

					HitType hit = HitType::kNone;

					if (a_event->flags & RE::TESHitEvent::Flag::kBashAttack) {
						// assume Melee
						hit = HitType::kMelee;
						LOG_4("{}[Events] [TESHitEvent] base attack");
					} else if (a_event->source == 0 && a_event->projectile == 0) {
						// assume H2H
						hit = HitType::kH2H;
						LOG_4("{}[Events] [TESHitEvent] source and projectile null");
					} else {
						RE::TESForm* source = RE::TESForm::LookupByID(a_event->source);
						RE::TESForm* projectile = RE::TESForm::LookupByID(a_event->projectile);

						switch (source->GetFormType()) {
						case RE::FormType::Weapon:
							if (RE::TESObjectWEAP* weap = source->As<RE::TESObjectWEAP>()) {
								if (weap->IsBow() || weap->IsCrossbow()) {
									hit = HitType::kRanged;
									LOG_4("{}[Events] [TESHitEvent] ranged weapon");
								} else if (weap->IsMelee()) {
									hit = HitType::kMelee;
									LOG_4("{}[Events] [TESHitEvent] melee weapon");
								} else if (weap->IsHandToHandMelee()) {
									hit = HitType::kH2H;
									LOG_4("{}[Events] [TESHitEvent] h2h");
								} else {
									hit = HitType::kMelee;
									LOG_4("{}[Events] [TESHitEvent] other weapon");
								}
							}
							// don't know about anything rn
						}
					}

					switch (hit) {
					case HitType::kNone:
						// no valid hit registered
						break;
					case HitType::kMelee:
						if (acagg->IsInfectedProgressing()) {
							for (int i = 0; i < Diseases::kMaxValue; i++) {
								auto disval = static_cast<Diseases::Disease>(i);
								if (acagg->IsInfectedProgressing(disval)) {
									auto dinfo = acagg->FindDisease(disval);
									if (dinfo) {
										auto dis = Main::data->GetDisease(disval);
										if (dis) {
											auto stage = dis->_stages[dinfo->stage];
											if (stage) {
												if (UtilityAlch::CalcChance(stage->_spreading[Spreading::kOnHitMelee].first))
													actar->AddDiseasePoints(disval, stage->_spreading[Spreading::kOnHitMelee].second);
											}
										}
									}
								}
							}
						}
						if (actar->IsInfected()) {
							for (int i = 0; i < Diseases::kMaxValue; i++) {
								auto disval = static_cast<Diseases::Disease>(i);
								if (acagg->IsInfected(disval)) {
									auto dinfo = acagg->FindDisease(disval);
									if (dinfo) {
										auto dis = Main::data->GetDisease(disval);
										if (dis) {
											auto stage = dis->_stages[dinfo->stage];
											if (stage) {
												if (UtilityAlch::CalcChance(stage->_spreading[Spreading::kGetHitMelee].first))
													acagg->AddDiseasePoints(disval, stage->_spreading[Spreading::kGetHitMelee].second);
											}
										}
									}
								}
							}
						}
						break;
					case HitType::kH2H:
						if (acagg->IsInfectedProgressing()) {
							for (int i = 0; i < Diseases::kMaxValue; i++) {
								auto disval = static_cast<Diseases::Disease>(i);
								if (acagg->IsInfectedProgressing(disval)) {
									auto dinfo = acagg->FindDisease(disval);
									if (dinfo) {
										auto dis = Main::data->GetDisease(disval);
										if (dis) {
											auto stage = dis->_stages[dinfo->stage];
											if (stage) {
												if (UtilityAlch::CalcChance(stage->_spreading[Spreading::kOnHitH2H].first))
													actar->AddDiseasePoints(disval, stage->_spreading[Spreading::kOnHitH2H].second);
											}
										}
									}
								}
							}
						}
						if (actar->IsInfected()) {
							for (int i = 0; i < Diseases::kMaxValue; i++) {
								auto disval = static_cast<Diseases::Disease>(i);
								if (acagg->IsInfected(disval)) {
									auto dinfo = acagg->FindDisease(disval);
									if (dinfo) {
										auto dis = Main::data->GetDisease(disval);
										if (dis) {
											auto stage = dis->_stages[dinfo->stage];
											if (stage) {
												if (UtilityAlch::CalcChance(stage->_spreading[Spreading::kGetHitH2H].first))
													acagg->AddDiseasePoints(disval, stage->_spreading[Spreading::kGetHitH2H].second);
											}
										}
									}
								}
							}
						}

						break;
					case HitType::kRanged:
						if (acagg->IsInfectedProgressing()) {
							for (int i = 0; i < Diseases::kMaxValue; i++) {
								auto disval = static_cast<Diseases::Disease>(i);
								if (acagg->IsInfectedProgressing(disval)) {
									auto dinfo = acagg->FindDisease(disval);
									if (dinfo) {
										auto dis = Main::data->GetDisease(disval);
										if (dis) {
											auto stage = dis->_stages[dinfo->stage];
											if (stage) {
												if (UtilityAlch::CalcChance(stage->_spreading[Spreading::kOnHitRanged].first))
													actar->AddDiseasePoints(disval, stage->_spreading[Spreading::kOnHitRanged].second);
											}
										}
									}
								}
							}
						}
						break;
					}
					if (hit != HitType::kNone) {
						if (acagg->ProgressAllDiseases() && Settings::Disease::_AllowActorDeath) {
							logusage("[Events] [HandleActors] Actor {} has died from their disease", Utility::PrintFormNonDebug(acagg));
							acagg->Kill();
						}
						if (actar->ProgressAllDiseases() && Settings::Disease::_AllowActorDeath) {
							logusage("[Events] [HandleActors] Actor {} has died from their disease", Utility::PrintFormNonDebug(actar));
							actar->Kill();
						}
					}
				}
			}
		}

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
	EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [EquipEvent]");

		if (a_event && a_event->actor.get() && a_event->equipped && a_event->baseObject)
		{
			RE::Actor* ac = a_event->actor->As<RE::Actor>();
			if (ac)
			{
				auto itr = Main::data->cureOptions.find(a_event->baseObject);
				if (itr != Main::data->cureOptions.end())
				{
					auto acinfo = Main::data->FindActorExisting(ac);
					if (acinfo->IsValid()) {
						for (auto dis : itr->second->diseases) {
							if (acinfo->IsInfected(dis))
							{
								acinfo->ApplyDiseaseModifier(dis, itr->second);
							}
						}
					}
				}
			}
		}

		return EventResult::kContinue;
	}

	EventResult EventHandler::ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*)
	{
		EvalProcessingEvent();
		if (a_event && a_event->actionRef.get() && a_event->objectActivated.get()) {
			RE::Actor* ac = a_event->actionRef->As<RE::Actor>();
			if (ac) {
				LOG_1("{}[Events] [TESActivateEvent]");
				if (Main::data->cureOptionsShrine) {
					if (Main::data->shrines.contains(a_event->objectActivated->GetFormID()) ||
						a_event->objectActivated->GetBaseObject() && Main::data->shrines.contains(a_event->objectActivated->GetBaseObject()->GetFormID())) {
						auto acinfo = Main::data->FindActorExisting(ac);
						if (acinfo->IsValid()) {
							for (auto dis : Main::data->cureOptionsShrine->diseases) {
								if (acinfo->IsInfected(dis)) {
									acinfo->ApplyDiseaseModifier(dis, Main::data->cureOptionsShrine.get());
								}
							}
						}
					}
				}
			}
		}

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
						if (acinfo->ForceIncreaseStage(Diseases::kAshWoeBlight) && Settings::Disease::_AllowActorDeath)
						{
							logusage("[Events] [HandleActors] Actor {} has died from their disease", Utility::PrintFormNonDebug(acinfo));
							acinfo->Kill();
						}
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
						console->Print((std::string("AlchExt: Printing target information: \t") + Utility::PrintFormNonDebug(acinfo->GetActor())).c_str());
						console->Print((std::string("Vampire: \t\t") + std::to_string(acinfo->IsVampire())).c_str());
						console->Print((std::string("Automaton: \t\t") + std::to_string(acinfo->IsAutomaton())).c_str());
						console->Print((std::string("Werewolf: \t\t") + std::to_string(acinfo->IsWerewolf())).c_str());
						console->Print((std::string("Printing disease information: \t")).c_str());
						for (auto dis : acinfo->diseases) {
							if (dis) {
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
					LOG_3("{}[Events] [InputEvent] registered infect wth sanguinare vampirism event");
					if (target) {
						Papyrus::Vampirism::InfectSanguinareVampirism(nullptr, 0, nullptr, target);
					} else {
						console->Print(std::string("AlchExt: Missing target").c_str());
					}
				} else if (key == 0x48) {  // numpad 8
					LOG_3("{}[Events] [InputEvent] registered print possible diseases event");
					if (target) {
						std::shared_ptr<ActorInfo> acinfo = Main::data->FindActor(target);
						console->Print((std::string("DO: Printing Infection information: \t") + Utility::PrintFormNonDebug(acinfo->GetActor())).c_str());
						auto [possible, forced] = Main::data->GetPossibleInfections(acinfo, nullptr);
						console->Print((std::string("Printing possible infections: \t")).c_str());
						for (auto& [key, value] : possible)
						{
							if (Main::data->GetDisease(key))
								console->Print((std::string("\tDisease:\t\t\t") + Main::data->GetDisease(key)->GetName()).c_str());
						}
						console->Print((std::string("Printing force infections: \t")).c_str());
						for (auto& dis : forced)
						{
							if (Main::data->GetDisease(dis))
								console->Print((std::string("\tDisease:\t\t\t") + Main::data->GetDisease(dis)->GetName()).c_str());
						}
					}
				} else if (key == 0x49) {  // numpad 9
				}
			}
		}
		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for end of fast travel
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESFastTravelEndEvent* a_event, RE::BSTEventSource<RE::TESFastTravelEndEvent>*)
	{
		// very important event. Allows to catch actors and other stuff that gets deleted, without dying, which could cause CTDs otherwise

		LOG_1("{}[Events] [TESFastTravelEndEvent]");

		Game::SetFastTraveling(false);
		Main::InitThreads();

		Main::RegisterFastTravelNPCs();

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
		scriptEventSourceHolder->GetEventSource<RE::TESActivateEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESActivateEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESFastTravelEndEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESFastTravelEndEvent).name())
		scriptEventSourceHolder->GetEventSource<RE::TESFormDeleteEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESFormDeleteEvent).name())
		RE::PlayerCharacter::GetSingleton()->AsBGSActorCellEventSource()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::BGSActorCellEvent).name());
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
