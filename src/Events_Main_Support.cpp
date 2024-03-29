
#include "ActorManipulation.h"
#include "Events.h"
#include "Game.h"
#include "Random.h"
#include "Utility.h"
#include "UtilityAlch.h"

namespace Events
{

#define EvalProcessing() \
	if (!CanProcess())   \
		return;


	/// <summary>
	/// initializes importent variables, which need to be initialized every time a game is loaded
	/// </summary>
	void Main::InitializeCompatibilityObjects()
	{ 
		if (!initialized)
			initialized = true;
	}


	/// <summary>
	/// Processes the item distribution for an actor
	/// </summary>
	/// <param name="acinfo"></param>
	void Main::ProcessDistribution(std::shared_ptr<ActorInfo> acinfo)
	{ /*
		// check wether this charackter maybe a follower
		if (acinfo->GetLastDistrTime() == 0.0f || RE::Calendar::GetSingleton()->GetDaysPassed() - acinfo->GetLastDistrTime() > 1) {
			if (!Distribution::ExcludedNPC(acinfo) && acinfo->IsDead() == false) {
				// begin with compatibility mode removing items before distributing new ones
				if (Settings::Debug::_CompatibilityRemoveItemsBeforeDist) {
					auto items = ACM::GetAllPotions(acinfo);
					auto it = items.begin();
					while (it != items.end()) {
						acinfo->RemoveItem(*it, 1);
						LOG1_1("{}[Events] [ProcessDistribution] Removed item {}", Utility::PrintForm(*it));
						it++;
					}
					items = ACM::GetAllPoisons(acinfo);
					it = items.begin();
					while (it != items.end()) {
						acinfo->RemoveItem(*it, 1);
						LOG1_1("{}[Events] [ProcessDistribution] Removed item {}", Utility::PrintForm(*it));
						it++;
					}
					items = ACM::GetAllFood(acinfo);
					it = items.begin();
					while (it != items.end()) {
						acinfo->RemoveItem(*it, 1);
						LOG1_1("{}[Events] [ProcessDistribution] Removed item {}", Utility::PrintForm(*it));
						it++;
					}
				}

				// if we have characters that should not get items, the function
				// just won't return anything, but we have to check for standard factions like CurrentFollowerFaction
				auto items = Distribution::GetDistrItems(acinfo);
				if (acinfo->IsDead()) {
					return;
				}
				if (items.size() > 0) {
					for (int i = 0; i < items.size(); i++) {
						if (items[i] == nullptr) {
							continue;
						}
						acinfo->AddItem(items[i], 1);
						LOG2_4("{}[Events] [ProcessDistribution] added item {} to actor {}", Utility::PrintForm(items[i]), Utility::PrintForm(acinfo));
					}
					acinfo->SetLastDistrTime(RE::Calendar::GetSingleton()->GetDaysPassed());
				}
			}
		}*/
	}

	void Main::ProcessInfections(std::shared_ptr<ActorInfo> acinfo)
	{
		float gamedays = RE::Calendar::GetSingleton()->gameDaysPassed->value;
		// either infections haven't been calculated or more than 30 days have passed and the actor is not infected anymore
		if (acinfo->ProcessedInitialInfections() == false || (gamedays - acinfo->ProcessedInitialInfectionsTime() > 30 && acinfo->IsInfected() == false)) {
			LOG1_1("{}[Events] [ProcessInfections] Actor {}", Utility::PrintForm(acinfo));
			/* auto tplt = Utility::ExtractTemplateInfo(acinfo->GetActor());*/
			auto [infections_possible, infections_forced_set] = data->GetPossibleInfections(acinfo, nullptr /*&tplt*/);
			std::vector<Diseases::Disease> infections_forced;
			try {
				std::for_each(infections_forced_set.begin(), infections_forced_set.end(), [&infections_forced](Diseases::Disease dis) { infections_forced.push_back(dis); });
			} catch (std::bad_alloc& e) {
				LOG_1("{}[Events] [ProcessInfections] Error");
				return;
			}

			acinfo->ProcessedInitialInfections(true);
			acinfo->ProcessedInitialInfectionsTime(gamedays);

			std::uniform_int_distribution<signed> rand5(1, 5);

			if (Settings::Infection::_AllowMultipleDiseases == false) {
				// if multiple infections aren't allowed, pick a random one from the force infections if there are any
				if (infections_forced.size() > 0) {
					std::uniform_int_distribution<signed> rand(0, infections_forced.size() - 1);
					int count = rand5(Random::rand);
					auto dis = infections_forced[rand(Random::rand)];
					LOG1_1("{}[Events] [ProcessInfections] Forced Infection {}", UtilityAlch::ToString(dis));
					for (int i = 1; i <= count; i++) {
						static_cast<void>(acinfo->ForceIncreaseStage(dis));
					}
					static_cast<void>(acinfo->ProgressDisease(dis, Random::rand1000(Random::rand)));
				} else if (infections_possible.size() > 0) {
					for (auto& [dis, value] : infections_possible) {
						LOG1_1("{}[Events] [ProcessInfections] Possible Infection {}", UtilityAlch::ToString(dis));
						auto& [chance, scale] = value;
						if (UtilityAlch::CalcChance(data->GetDisease(dis)->_baseInfectionChance))
						{
							int count = rand5(Random::rand);
							for (int i = 1; i <= count; i++) {
								static_cast<void>(acinfo->ForceIncreaseStage(dis));
							}
							static_cast<void>(acinfo->ProgressDisease(dis, Random::rand1000(Random::rand)));
							return;
						}
					}
				}
			}
			else
			{
				// if multiple infections are allowed process them

				// first remove forced infections from the possible ones
				for (auto dis : infections_forced_set)
					infections_possible.erase(dis);

				// process forced infections
				for (auto dis : infections_forced_set) {
					LOG1_1("{}[Events] [ProcessInfections] Forced Infection {}", UtilityAlch::ToString(dis));
					int count = rand5(Random::rand);
					for (int i = 1; i <= count; i++)
						static_cast<void>(acinfo->ForceIncreaseStage(dis));
				}
				// process possible infections
				for (auto& [dis, value] : infections_possible) {
					if (data->GetDisease(dis)) {
						auto& [chance, scale] = value;
						LOG1_1("{}[Events] [ProcessInfections] Possible Infection {}", UtilityAlch::ToString(dis));
						auto dostuff = [&acinfo, &dis, &rand5]() {
							int count = rand5(Random::rand);
							for (int i = 1; i <= count; i++)
								static_cast<void>(acinfo->ForceIncreaseStage(dis));
							static_cast<void>(acinfo->ProgressDisease(dis, Random::rand1000(Random::rand)));
						};
						if (chance == 0 && scale == 0) {
							
							if (UtilityAlch::CalcChance(data->GetDisease(dis)->_baseInfectionChance))
								dostuff();
						} else if (chance == 0 && scale != 0) {
							if (UtilityAlch::CalcChance(scale * data->GetDisease(dis)->_baseInfectionChance))
								dostuff();
						} else if (chance != 0) {
							if (UtilityAlch::CalcChance(chance))
								dostuff();
						}

						/* if (UtilityAlch::CalcChance(chance == 0 ? scale == 0 ? data->GetDisease(dis)->_baseInfectionChance : scale * data->GetDisease(dis)->_baseInfectionChance : chance)) {
							int count = rand5(Random::rand);
							for (int i = 1; i <= count; i++)
								static_cast<void>(acinfo->ForceIncreaseStage(dis));
							static_cast<void>(acinfo->ProgressDisease(dis, Random::rand1000(Random::rand)));
						}*/
					}
				}
			}
		}
	}

	
	/// <summary>
	/// Registers an NPC for handling
	/// </summary>
	/// <param name="actor"></param>
	void Main::RegisterNPC(RE::Actor* actor)
	{
		EvalProcessing();
		// exit if the actor is unsafe / not valid
		if (Utility::ValidateActor(actor) == false)
			return;
		// if currently fasttraveling, save actor to register later
		if (Game::IsFastTravelling()) {
			LOG1_1("{}[Events] [RegisterNPC] Saving for later: {}", Utility::PrintForm(actor));
			toregister.push_back(actor->GetHandle());
			LOG_1("{}[Events] [RegisterNPC] Saved");
			return;
		}
		LOG1_1("{}[Events] [RegisterNPC] Trying to register new actor for potion tracking: {}", Utility::PrintForm(actor));
		std::shared_ptr<ActorInfo> acinfo = data->FindActor(actor);
		std::shared_ptr<CellInfo> cinfo = data->FindCell(actor->GetParentCell());
		LOG1_1("{}[Events] [RegisterNPC] Found: {}", Utility::PrintForm(acinfo));
		// if actor was dead, exit
		if (acinfo->GetDead()) {
			LOG_1("{}[Events] [RegisterNPC] Actor already dead");
			return;
		}
		// reset object to account for changes to underlying objects
		acinfo->Reset(actor);
		if (acinfo->IsValid() == false) {
			LOG_1("{}[Events] [RegisterNPC] Actor reset failed");
			return;
		}
		// find out whether to insert the actor, if yes insert him into the temp insert list
		sem.acquire();
		if (!acset.contains(acinfo)) {
			acset.insert(acinfo);
		} else {
			sem.release();
			LOG_1("{}[Events] [RegisterNPC] Actor already registered");
			return;
		}
		//if (!cellist.contains(cinfo)) {
		//	cellist.insert(cellist.begin(), cinfo);
		//}
		sem.release();

		//acinfo->ForceIncreaseStage(Diseases::kAshWoeBlight);

		ProcessDistribution(acinfo);
		ProcessInfections(acinfo);
		EvalProcessing();
		if (actor->IsDead())
			return;

		LOG_1("{}[Events] [RegisterNPC] finished registering NPC");
	}

	/// <summary>
	/// Registers NPCs that could not be registered during fast travel
	/// </summary>
	void Main::RegisterFastTravelNPCs()
	{
		RE::Actor* reg = nullptr;

		while (!toregister.empty()) {
			reg = toregister.front().get().get();
			toregister.pop_front();
			RegisterNPC(reg);
		}
	}

	/// <summary>
	/// Unregisters an NPC form handling
	/// </summary>
	/// <param name="actor"></param>
	void Main::UnregisterNPC(RE::Actor* actor)
	{
		EvalProcessing();
		// exit if actor is unsafe / not valid
		if (Utility::ValidateActor(actor) == false)
			return;
		LOG1_1("{}[Events] [UnregisterNPC] Unregister NPC from potion tracking: {}", Utility::PrintForm(actor));
		std::shared_ptr<ActorInfo> acinfo = data->FindActor(actor);
		sem.acquire();
		acset.erase(acinfo);
		sem.release();
		LOG_1("{}[Events] [UnregisterNPC] Unregistered NPC");
	}

	/// <summary>
	/// Unregisters an NPC from handling
	/// </summary>
	/// <param name="acinfo"></param>
	void Main::UnregisterNPC(std::shared_ptr<ActorInfo> acinfo)
	{
		EvalProcessing();
		LOG1_1("{}[Events] [UnregisterNPC] Unregister NPC from potion tracking: {}", acinfo->GetName());
		sem.acquire();
		acset.erase(acinfo);
		sem.release();
	}

	/// <summary>
	/// Unregisters an NPC from handling
	/// </summary>
	/// <param name="acinfo"></param>
	void Main::UnregisterNPC(RE::FormID formid)
	{
		EvalProcessing();
		LOG1_1("{}[Events] [UnregisterNPC] Unregister NPC from potion tracking: {}", Utility::GetHex(formid));
		sem.acquire();
		auto itr = acset.begin();
		while (itr != acset.end()) {
			if (std::shared_ptr<ActorInfo> acinfo = itr->lock()) {
				if (acinfo->GetFormIDBlank() == formid) {
					acset.erase(itr);
					break;
				}
			} else {
				// weak pointer is expired, so remove it while we are on it
				acset.erase(itr);
			}
			itr++;
		}
		sem.release();
	}

	bool Main::IsDead(RE::Actor* actor)
	{
		return actor == nullptr || deads.contains(actor->GetHandle()) || actor->IsDead();
	}

	bool Main::IsDeadEventFired(RE::Actor* actor)
	{
		return actor == nullptr || deads.contains(actor->GetHandle());
	}

	void Main::SetDead(RE::Actor* actor)
	{
		if (actor != nullptr)
			deads.insert(actor->GetHandle());
	}
}
