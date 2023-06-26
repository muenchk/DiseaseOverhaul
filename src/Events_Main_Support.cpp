
#include "ActorManipulation.h"
#include "Events.h"
#include "Utility.h"

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
		if (!cellist.contains(cinfo)) {
			cellist.insert(cellist.begin(), cinfo);
		}
		sem.release();

		ProcessDistribution(acinfo);
		EvalProcessing();
		if (actor->IsDead())
			return;

		LOG_1("{}[Events] [RegisterNPC] finished registering NPC");
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