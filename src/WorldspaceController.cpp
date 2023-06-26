#include "Events.h"
#include "Logging.h"
#include "UtilityAlch.h"
#include "WorldspaceController.h"


void World::Init()
{
	data = Data::GetSingleton();

	// init game objects
	playerRef = RE::PlayerCharacter::GetSingleton();
}

/// <summary>
/// returns singleton to the World
/// </summary>
/// <returns></returns>
World* World::GetSingleton()
{
	static World singleton;
	return std::addressof(singleton);
}

void World::Reset()
{
	playerCell = nullptr;
	playerWorldspace = nullptr;
	playerCellID = 0;
}

void World::PlayerChangeCell()
{
	LOG_1("{}[World] [PlayerChangeCell]");
	// if cell has changed, update cells
	if (playerCell != playerRef->GetParentCell()) {
		playerCell = playerRef->GetParentCell();
		playerCellID = playerRef->GetParentCell()->GetFormID();
		if (playerWorldspace != playerCell->GetRuntimeData().worldSpace) {
			playerWorldspace = playerCell->GetRuntimeData().worldSpace;
			auto regis = [this](RE::TESObjectREFR& ref) {
				RE::Actor* actor = ref.As<RE::Actor>();
				if (Utility::ValidateActor(actor) && actor->IsDead() == false && !actor->IsPlayerRef()) {
					Events::Main::RegisterNPC(actor);
				}
				return RE::BSContainer::ForEachResult::kContinue;
			};
			if (playerWorldspace && playerWorldspace->persistentCell)
				for (auto& ptr : playerWorldspace->persistentCell->GetRuntimeData().references) {
					RE::Actor* actor = ptr.get()->As<RE::Actor>();
					if (Utility::ValidateActor(actor) && !Events::Main::IsDead(actor) && !actor->IsPlayerRef()) {
						if (Distribution::ExcludedNPCFromHandling(actor) == false)
							Events::Main::RegisterNPC(actor);
					}
				}
				//playerWorldspace->persistentCell->ForEachReference(regis);
		}
	} else {
		LOG_1("{}[World] [PlayerChangeCell] playercell is null");
	}
}

void World::PlayerChangeCell(RE::TESObjectCELL* cell)
{
	LOG_1("{}[World] [PlayerChangeCell]");
	if (cell != nullptr) {
		LOG2_1("{}[World] [PlayerChangeCell] World1 {}\tWorld2 {}", Utility::PrintForm(playerWorldspace), Utility::PrintForm(cell->GetRuntimeData().worldSpace));
		if (playerCell != cell) {
			if (playerWorldspace != cell->GetRuntimeData().worldSpace) {
				auto unregis = [this](RE::TESObjectREFR& ref) {
					RE::Actor* actor = ref.As<RE::Actor>();
					if (Utility::ValidateActor(actor) && actor->IsDead() == false && !actor->IsPlayerRef()) {
						Events::Main::UnregisterNPC(actor);
					}
					return RE::BSContainer::ForEachResult::kContinue;
				};
				if (playerWorldspace && playerWorldspace->persistentCell)
					for (auto& ptr : playerWorldspace->persistentCell->GetRuntimeData().references) {
						RE::Actor* actor = ptr.get()->As<RE::Actor>();
						if (Utility::ValidateActor(actor) && !Events::Main::IsDead(actor) && !actor->IsPlayerRef()) {
							if (Distribution::ExcludedNPCFromHandling(actor) == false)
								Events::Main::UnregisterNPC(actor);
						}
					}
					//playerWorldspace->persistentCell->ForEachReference(unregis);
				playerWorldspace = cell->GetRuntimeData().worldSpace;
				auto regis = [this](RE::TESObjectREFR& ref) {
					RE::Actor* actor = ref.As<RE::Actor>();
					if (Utility::ValidateActor(actor) && actor->IsDead() == false && !actor->IsPlayerRef()) {
						Events::Main::RegisterNPC(actor);
					}
					return RE::BSContainer::ForEachResult::kContinue;
				};
				if (playerWorldspace && playerWorldspace->persistentCell)
					for (auto& ptr : playerWorldspace->persistentCell->GetRuntimeData().references) {
						RE::Actor* actor = ptr.get()->As<RE::Actor>();
						if (Utility::ValidateActor(actor) && !Events::Main::IsDead(actor) && !actor->IsPlayerRef()) {
							if (Distribution::ExcludedNPCFromHandling(actor) == false)
								Events::Main::RegisterNPC(actor);
						}
					}
					//playerWorldspace->persistentCell->ForEachReference(regis);
			}
		}
		playerCell = cell;
		playerCellID = cell->GetFormID();
	} else {
		LOG_1("{}[World] [PlayerChangeCell] cell is null");
	}
}

void World::PlayerChangeCell(RE::FormID cellid)
{
	LOG_1("{}[World] [PlayerChangeCell]");
	if (cellid != 0) {
		RE::TESObjectCELL* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(cellid);
		if (cell != nullptr) {
			if (playerCell != cell) {
				if (playerWorldspace != cell->GetRuntimeData().worldSpace) {
					auto unregis = [this](RE::TESObjectREFR& ref) {
						RE::Actor* actor = ref.As<RE::Actor>();
						if (Utility::ValidateActor(actor) && actor->IsDead() == false && !actor->IsPlayerRef()) {
							Events::Main::UnregisterNPC(actor);
						}
						return RE::BSContainer::ForEachResult::kContinue;
					};
					if (playerWorldspace && playerWorldspace->persistentCell)
						for (auto& ptr : playerWorldspace->persistentCell->GetRuntimeData().references) {
							RE::Actor* actor = ptr.get()->As<RE::Actor>();
							if (Utility::ValidateActor(actor) && !Events::Main::IsDead(actor) && !actor->IsPlayerRef()) {
								if (Distribution::ExcludedNPCFromHandling(actor) == false)
									Events::Main::UnregisterNPC(actor);
							}
						}
						//layerWorldspace->persistentCell->ForEachReference(unregis);
					playerWorldspace = cell->GetRuntimeData().worldSpace;
					auto regis = [this](RE::TESObjectREFR& ref) {
						RE::Actor* actor = ref.As<RE::Actor>();
						if (Utility::ValidateActor(actor) && actor->IsDead() == false && !actor->IsPlayerRef()) {
							Events::Main::RegisterNPC(actor);
						}
						return RE::BSContainer::ForEachResult::kContinue;
					};
					if (playerWorldspace && playerWorldspace->persistentCell)
						for (auto& ptr : playerWorldspace->persistentCell->GetRuntimeData().references) {
							RE::Actor* actor = ptr.get()->As<RE::Actor>();
							if (Utility::ValidateActor(actor) && !Events::Main::IsDead(actor) && !actor->IsPlayerRef()) {
								if (Distribution::ExcludedNPCFromHandling(actor) == false)
									Events::Main::RegisterNPC(actor);
							}
						}
						//playerWorldspace->persistentCell->ForEachReference(regis);
				}
			}
			playerCell = cell;
			playerCellID = cell->GetFormID();
		} else {
			LOG_1("{}[World] [PlayerChangeCell] cellid is unknown");
		}
	} else {
		LOG_1("{}[World] [PlayerChangeCell] cellid is null");
	}
}
