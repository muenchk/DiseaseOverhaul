#include "ActorInfo.h"
#include "Settings.h"
#include "UtilityAlch.h"
#include "Distribution.h"
#include "BufferOperations.h"
#include "ActorManipulation.h"
#include "Papyrus.h"
#include "Random.h"
#include "Stats.h"
#include "VM.h"

#pragma region Disease

void ActorInfo::DeleteDisease(Diseases::Disease value)
{
	auto dinfo = diseases[value];
	if (dinfo != nullptr) {
		diseases[value].reset();
	}
}

void ActorInfo::AddDiseasePoints(Diseases::Disease disval, float points)
{
#pragma omp atomic update
	diseasepoints[disval] += points;
	loginfo("[ActorInfo] [AddDiseasePoints] Actor: {}, Disease: {}, Points: {}", _formstring, UtilityAlch::ToString(disval), points);
}

bool ActorInfo::ProgressAllDiseases()
{
	bool kill = false;
	for (int i = 0; i < Diseases::kMaxValue; i++) {
		if (diseasepoints[i] > 0)
			kill |= ProgressDisease(static_cast<Diseases::Disease>(i), diseasepoints[i]);
		diseasepoints[i] = 0;
	}
	return kill;
}
std::shared_ptr<DiseaseInfo> ActorInfo::FindDisease(Diseases::Disease value)
{
	return diseases[value];
}

bool ActorInfo::ProgressDisease(Diseases::Disease value, float points)
{
	if (!valid || dead)
		return false;

	// sanguinare vampirism is only applicable to the player
	// also return if the actor already is a vampire
	if (value == Diseases::Disease::kSanguinareVampirism && (!IsPlayer() || _vampire))
		return false;

	LOG3_3("{}[ActorInfo] [ProgressDisease] actor: {},disease: {}, points: {}", _formstring, UtilityAlch::ToString(value), points);

	// stat tracking
	Stats::DiseaseStats_ProgressDisease++;

	// init data if not already happened

	bool killac = false;

	// if the points are 0 just return
	if (points == 0)
		return false;

	std::shared_ptr<DiseaseInfo> dinfo = FindDisease(value);
	if (!dinfo) {
		// if there is no disease info and we want to loose points, then there is no meaning in creating info at all
		if (points < 0)
			return false;
		dinfo = std::make_shared<DiseaseInfo>();
		dinfo->advPoints = 0;
		dinfo->disease = value;
		dinfo->earliestAdvancement = 0;
		dinfo->immuneUntil = 0;
		dinfo->permanentModifiers = 0;
		dinfo->permanentModifiersPoints = 0;
		dinfo->stage = 0;
		dinfo->status = DiseaseStatus::kInfection;
		diseases[value] = dinfo;
	}

	float currentgameday = RE::Calendar::GetSingleton()->GetDaysPassed();

	// check for immunity, if immune don't add points, just deduct
	if (dinfo->immuneUntil > currentgameday)
		return false;

	LOG_4("{}[ActorInfo] [ProgressDisease] start work");

	// do actual work
	std::shared_ptr<Disease> dis = data->GetDisease(value);
	dinfo->advPoints += points;
	switch (dinfo->status) {
	case DiseaseStatus::kInfection:
		LOG_4("{}[ActorInfo] [ProgressDisease] Infection");
		// infection stage means no effects
		// we have to check whether we jump out of infection stage and calculate effects
		if (dinfo->advPoints > dis->_stageInfection->_advancementThreshold) {
			LOG_4("{}[ActorInfo] [ProgressDisease] advance");
			// advance to incubation
			dinfo->stage = 0;
			dinfo->status = DiseaseStatus::kProgressing;
			// carry over points up to half of the next stage
			dinfo->advPoints -= dis->_stageInfection->_advancementThreshold;  // carry over excessive infection points
			if (dinfo->advPoints > (dis->_stages[0]->_advancementThreshold / 2))
				dinfo->advPoints = (float)dis->_stages[0]->_advancementThreshold / 2;
			dinfo->earliestAdvancement = currentgameday + dis->_stages[0]->_advancementTime;
			dinfo->permanentModifiers = 0;
			dinfo->permanentModifiersPoints = 0;
			// apply effect if there is one
			if (dis->_stages[0]->effect != nullptr)
				AddSpell(dis->_stages[dinfo->stage]->effect);
			//acinfo->CastSpell(false, 0, dis->_stages[0]->effect);
			//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[0]->effect, true, actor, 100, false, false, actor);
		} else if (dinfo->advPoints <= 0) {
			LOG_4("{}[ActorInfo] [ProgressDisease] delete info");
			// we had a regressing effect, so go down stages
			// in essence we don't have an infection anymore, so remove dieseaseinfo
			DeleteDisease(value);
		}
		break;
	case DiseaseStatus::kProgressing:
		LOG_4("{}[ActorInfo] [ProgressDisease] Progressing");
		// this is more tricky, first check current stage and then advancement
		if (dinfo->advPoints > dis->_stages[dinfo->stage]->_advancementThreshold) {
			// try advance stage
			// if we have reached the maximum stage, calculate endeffects set return value to true if actor should die and then calculate rest as normal
			if (dinfo->stage == dis->_numstages) {
				LOG_4("{}[ActorInfo] [ProgressDisease] max stage exceeded");
				// calculate end effects

				// apply end effect
				if (dis->endeffect != nullptr)
					AddSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->CastSpell(false, 0, dis->endeffect);
				//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->endeffect, true, actor, 100, false, false, actor);
				// apply endevents
				if (dis->endevents & DiseaseEndEvents::kDie100) {
					killac = true;
				}
				if (dis->endevents & DiseaseEndEvents::kDie75) {
					if (Random::rand100(Random::rand) < 75)
						killac = true;
				}
				if (dis->endevents & DiseaseEndEvents::kDie50) {
					if (Random::rand100(Random::rand) < 50)
						killac = true;
				}
				if (dis->endevents & DiseaseEndEvents::kDie25) {
					if (Random::rand100(Random::rand) < 25)
						killac = true;
				}
				// if we are allowed to enter regression do so
				if ((dis->endevents & DiseaseEndEvents::kNoRegression) == 0) {
					// enter regression
					dinfo->status = DiseaseStatus::kRegressing;
					// we do not have a timer for going down stages so reset this one
					dinfo->earliestAdvancement = 0;
					// dont't change anything else, we are regressing from the current point
				}

			} else if (currentgameday > dinfo->earliestAdvancement || Settings::Disease::_ignoreTimeAdvancementConstraint) {
				LOG_4("{}[ActorInfo] [ProgressDisease] advance");
				// advance to next stage
				// remove last effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					RemoveSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->DispelEffect(dis->_stages[dinfo->stage]->effect);
				//actor->AsMagicTarget()->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
				if (dinfo->stage == 0)  //we are in incubation stage
				{
					// we are allowed to jump more than one stage here, straight up to max stage 3 whch is idx [2]
					while (dinfo->advPoints > dis->_stages[dinfo->stage]->_advancementThreshold && dinfo->stage < 2) {
						dinfo->advPoints -= dis->_stages[dinfo->stage]->_advancementThreshold;
						dinfo->stage++;
					}
				} else {
					dinfo->advPoints -= dis->_stages[dinfo->stage]->_advancementThreshold;
					dinfo->stage++;
					// only carry over half the points needed for next stage
					if (dinfo->advPoints > (dis->_stages[dinfo->stage]->_advancementThreshold / 2))
						dinfo->advPoints = (float)dis->_stages[dinfo->stage]->_advancementThreshold / 2;
				}
				dinfo->earliestAdvancement = currentgameday + dis->_stages[dinfo->stage]->_advancementTime;
				// apply effect if there is one
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					AddSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->CastSpell(false, 0, dis->_stages[dinfo->stage]->effect);
				//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
			}
			// else dont advance just accumulate points until advancement
		} else if (dinfo->advPoints < 0) {
			// we had a regressing effect so go down stages
			if (dinfo->stage <= 1) {  // incubation stage does not exist for regression
				LOG_4("{}[ActorInfo] [ProgressDisease] gain immunity");
				// we are already in the lowest stage, so go back to infection and gain immunity
				dinfo->status = DiseaseStatus::kInfection;
				dinfo->earliestAdvancement = 0;
				dinfo->immuneUntil = currentgameday + dis->immunityTime;
				dinfo->permanentModifiers = 0;
				dinfo->permanentModifiersPoints = 0;
				// remove effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					RemoveSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->DispelEffect(dis->_stages[dinfo->stage]->effect);
				//actor->AsMagicTarget()->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
			} else {
				LOG_4("{}[ActorInfo] [ProgressDisease] devance");
				// we just regress one stage
				// remove last effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					RemoveSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->DispelEffect(dis->_stages[dinfo->stage]->effect);
				//actor->AsMagicTarget()->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
				dinfo->stage--;
				dinfo->advPoints = (float)dis->_stages[dinfo->stage]->_advancementThreshold;
				// apply effect if there is one
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					AddSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->CastSpell(false, 0, dis->_stages[dinfo->stage]->effect);
				//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
			}
		}
		break;
	case DiseaseStatus::kRegressing:
		LOG_4("{}[ActorInfo] [ProgressDisease] Regressing");
		// if you are regressing you cannot progress anymore, even if you were to gain advPoints
		if (dinfo->advPoints < 0) {
			// we had a regressing effect so go down stages
			if (dinfo->stage <= 1) {  // incubation stage does not exist for regression
				LOG_4("{}[ActorInfo] [ProgressDisease] gain immunity");
				// we are already in the lowest stage, so go back to infection and gain immunity
				dinfo->status = DiseaseStatus::kInfection;
				dinfo->earliestAdvancement = 0;
				dinfo->immuneUntil = currentgameday + dis->immunityTime;
				dinfo->permanentModifiers = 0;
				dinfo->permanentModifiersPoints = 0;
				// remove effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					RemoveSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->DispelEffect(dis->_stages[dinfo->stage]->effect);
				//actor->AsMagicTarget()->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
			} else {
				LOG_4("{}[ActorInfo] [ProgressDisease] devance");
				// we just regress one stage
				// remove last effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					RemoveSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->DispelEffect(dis->_stages[dinfo->stage]->effect);
				//actor->AsMagicTarget()->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
				dinfo->stage--;
				dinfo->advPoints = (float)dis->_stages[dinfo->stage]->_advancementThreshold;
				// apply effect if there is one
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					AddSpell(dis->_stages[dinfo->stage]->effect);
				//acinfo->CastSpell(false, 0, dis->_stages[dinfo->stage]->effect);
				//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
			}
		}
		break;
	}

	// handle vampirism
	if (value == Diseases::Disease::kSanguinareVampirism)
	{
		if (dinfo->stage == dis->_numstages)
		{
			// if we have reached the maximum stage turn the actor into a vampire

			//If Game.GetPlayer().GetCombatState() == 0 && Game.IsMovementControlsEnabled() && Game.IsFightingControlsEnabled() VampireSleepMessage.Show();
			//Debug.Trace(self + "Player not in combat, and controls are enabled. Trigger Vampire change")
			//	PlayerVampireQuest.VampireChange(Game.GetPlayer())
			//		Else

			LOG1_4("{}[ActorInfo] [ProgressDisease] Preparing for vampire transformation for actor {}", _formstring);
			if (IsInCombat() == false)
			{
				RE::TESQuest* qs = RE::TESForm::LookupByID<RE::TESQuest>(0xEAFD5);
				if (qs) {
					LOG_4("{}[ActorInfo] [ProgressDisease] Firing vampire transformation");
					auto scriptobj = ScriptObject::FromForm(qs, "PlayerVampireQuestScript");
					auto args = RE::MakeFunctionArguments(GetActor());
					ScriptCallbackPtr nullCallback;
					Papyrus::VM->DispatchMethodCall(scriptobj, "VampireChange"sv, args, nullCallback);
					DeleteDisease(Diseases::Disease::kSanguinareVampirism);

					// get vampire message
					RE::BGSMessage* vampsleepmes = RE::TESForm::LookupByID<RE::BGSMessage>(0x0ED0AB);
					if (vampsleepmes) {
						auto vsmscr = ScriptObject::FromForm(vampsleepmes, "Message");
						auto argus = RE::MakeFunctionArguments(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

						bool result = Papyrus::VM->DispatchMethodCall(vsmscr, "Show"sv, argus, nullCallback);
						LOG1_4("{}[ActorInfo] [ProgressDisease] Dispatched Message call: {}", result);
					}

					// dispel sanguinare vampirism script effect
					DispelEffect(RE::TESForm::LookupByID<RE::SpellItem>(0xB8780));

					// update npc vampire status
					_vampire = true;
				}
			}
		}
	}

	CalcDiseaseFlags();
	LOG_4("{}[ActorInfo] [ProgressDisease] end");
	return killac;
}

bool ActorInfo::ForceIncreaseStage(Diseases::Disease value)
{
	LOG_2("{}[ActorInfo] [ForceIncreaseStage]");

	bool killac = false;

	// if actor is already a vampire, return
	if (value == Diseases::Disease::kSanguinareVampirism && _vampire)
		return false;

	std::shared_ptr<DiseaseInfo> dinfo = FindDisease(value);
	if (!dinfo) {
		dinfo = std::make_shared<DiseaseInfo>();
		dinfo->advPoints = 0;
		dinfo->disease = value;
		dinfo->earliestAdvancement = 0;
		dinfo->immuneUntil = 0;
		dinfo->permanentModifiers = 0;
		dinfo->permanentModifiersPoints = 0;
		dinfo->stage = 0;
		dinfo->status = DiseaseStatus::kInfection;
		diseases[value] = dinfo;
	}

	float currentgameday = RE::Calendar::GetSingleton()->GetDaysPassed();

	// do actual work
	std::shared_ptr<Disease> dis = data->GetDisease(value);
	switch (dinfo->status) {
	case DiseaseStatus::kInfection:
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] Infection");
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] advance");
		// advance to incubation
		dinfo->stage = 0;
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] 1");
		dinfo->status = DiseaseStatus::kProgressing;
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] 2");
		dinfo->advPoints = 0;
		LOG1_4("{}[ActorInfo] [ForceIncreaseStage] 3 {}", (uint64_t)dis->_stages);
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] 4");
		dinfo->earliestAdvancement = currentgameday + dis->_stages[0]->_advancementTime;
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] 5");
		dinfo->permanentModifiers = 0;
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] 6");
		dinfo->permanentModifiersPoints = 0;
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] 7");
		// apply effect if there is one
		if (dis->_stages[0]->effect != nullptr)
			AddSpell(dis->_stages[0]->effect);
			//CastSpell(false, 0, dis->_stages[dinfo->stage]->effect);
		//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[0]->effect, true, actor, 100, false, false, actor);
		//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->SpellCast(false, 0, dis->_stages[0]->effect);
		else
			LOG_4("{}[ActorInfo] [ForceIncreaseStage] the stage does not have any effect");
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] 8");
		break;
	case DiseaseStatus::kProgressing:
	case DiseaseStatus::kRegressing:
		LOG_4("{}[ActorInfo] [ForceIncreaseStage] Gressing");
		if (dinfo->stage == dis->_numstages) {
			LOG_4("{}[ActorInfo] [ForceIncreaseStage] max stage exceeded");
			// calculate end effects

			// apply end effect
			if (dis->endeffect != nullptr)
				//CastSpell(false, 0, dis->_stages[dinfo->stage]->effect);
				AddSpell(dis->endeffect);
			//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->endeffect, true, actor, 100, false, false, actor);
			//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->SpellCast(false, 0, dis->endeffect);
			else
				LOG_4("{}[ActorInfo] [ForceIncreaseStage] there is no endeffect");
			// apply endevents
			if (dis->endevents & DiseaseEndEvents::kDie100) {
				killac = true;
			}
			if (dis->endevents & DiseaseEndEvents::kDie75) {
				if (Random::rand100(Random::rand) < 75)
					killac = true;
			}
			if (dis->endevents & DiseaseEndEvents::kDie50) {
				if (Random::rand100(Random::rand) < 50)
					killac = true;
			}
			if (dis->endevents & DiseaseEndEvents::kDie25) {
				if (Random::rand100(Random::rand) < 25)
					killac = true;
			}

			// enter regression
			dinfo->status = DiseaseStatus::kRegressing;
			// we do not have a timer for going down stages so reset this one
			dinfo->earliestAdvancement = 0;
			// dont't change anything else, we are regressing from the current point

		} else {
			LOG_4("{}[ActorInfo] [ForceIncreaseStage] advance");
			// advance to next stage
			// remove last effect
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				RemoveSpell(dis->_stages[dinfo->stage]->effect);
				//DispelEffect(dis->_stages[dinfo->stage]->effect);
			//acinfo->DispelEffect(dis->_stages[dinfo->stage]->effect);
			//actor->AsMagicTarget()->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
			dinfo->advPoints = 0;
			dinfo->stage++;
			dinfo->earliestAdvancement = currentgameday + dis->_stages[dinfo->stage]->_advancementTime;
			// apply effect if there is one
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				AddSpell(dis->_stages[dinfo->stage]->effect);
				//CastSpell(false, 0, dis->_stages[dinfo->stage]->effect);
			//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
			//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->SpellCast(false, 0, dis->_stages[dinfo->stage]->effect);
			else
				LOG_4("{}[ActorInfo] [ForceIncreaseStage] the stage does not have any effect");
		}
		break;
	}
	CalcDiseaseFlags();
	LOG_4("{}[ActorInfo] [ForceIncreaseStage] end");
	return killac;
}

void ActorInfo::ForceDecreaseStage(Diseases::Disease value)
{
	LOG_2("{}[ActorInfo] [ForceDecreaseStage]");

	bool killac = false;

	std::shared_ptr<DiseaseInfo> dinfo = FindDisease(value);
	if (!dinfo) {
		dinfo = std::make_shared<DiseaseInfo>();
		dinfo->advPoints = 0;
		dinfo->disease = value;
		dinfo->earliestAdvancement = 0;
		dinfo->immuneUntil = 0;
		dinfo->permanentModifiers = 0;
		dinfo->permanentModifiersPoints = 0;
		dinfo->stage = 0;
		dinfo->status = DiseaseStatus::kInfection;
	}

	float currentgameday = RE::Calendar::GetSingleton()->GetDaysPassed();

	// do actual work
	std::shared_ptr<Disease> dis = data->GetDisease(value);
	switch (dinfo->status) {
	case DiseaseStatus::kInfection:
		LOG_4("{}[ActorInfo] [ForceDecreaseStage] Infection");
		LOG_4("{}[ActorInfo] [ForceDecreaseStage] delete info");
		{
			// we had a regressing effect, so go down stages
			// in essence we don't have an infection anymore, so remove dieseaseinfo
			DeleteDisease(value);
		}
		break;
	case DiseaseStatus::kProgressing:
	case DiseaseStatus::kRegressing:
		LOG_4("{}[ActorInfo] [ForceDecreaseStage] Gressing");
		if (dinfo->stage <= 1) {  // incubation stage does not exist for regression
			LOG_4("{}[ActorInfo] [ForceDecreaseStage] gain immunity");
			// we are already in the lowest stage, so go back to infection and gain immunity
			dinfo->status = DiseaseStatus::kInfection;
			dinfo->earliestAdvancement = 0;
			dinfo->immuneUntil = currentgameday + dis->immunityTime;
			dinfo->permanentModifiers = 0;
			dinfo->permanentModifiersPoints = 0;
			// remove effect
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				RemoveSpell(dis->_stages[dinfo->stage]->effect);
			//acinfo->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
		} else {
			LOG_4("{}[ActorInfo] [ForceDecreaseStage] devance");
			// we just regress one stage
			// remove last effect
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				RemoveSpell(dis->_stages[dinfo->stage]->effect);
			//actor->AsMagicTarget()->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
			dinfo->stage--;
			dinfo->advPoints = (float)dis->_stages[dinfo->stage]->_advancementThreshold;
			// apply effect if there is one
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				AddSpell(dis->_stages[dinfo->stage]->effect);
			//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
		}
		break;
	}
	CalcDiseaseFlags();
	LOG_4("{}[ActorInfo] [ForceDecreaseStage] end");
}

bool ActorInfo::IsInfected()
{
	return disflags > 0;
}

bool ActorInfo::IsInfected(Diseases::Disease dis)
{
	return (disflags & ((uint64_t)1 << static_cast<EnumType>(dis))) > 0;
}

bool ActorInfo::IsInfectedProgressing()
{
	return disflagsprog > 0;
}

bool ActorInfo::IsInfectedProgressing(Diseases::Disease dis)
{
	return (disflagsprog & ((uint64_t)1 << static_cast<EnumType>(dis))) > 0;
}

void ActorInfo::CleanDiseases()
{
	for (int i = 0; i < Diseases::kMaxValue; i++){
		auto dinfo = diseases[i];
		if (dinfo != nullptr && dinfo->status == DiseaseStatus::kInfection && RE::Calendar::GetSingleton()->GetDaysPassed() > dinfo->immuneUntil && dinfo->advPoints == 0) {
			diseases[i].reset();
		}
	}
}

void ActorInfo::CalcDiseaseFlags()
{
	disflags = 0;
	disflagsprog = 0;
	for (auto dis : diseases) {
		if (dis) {
			if (dis->status == DiseaseStatus::kProgressing) {
				disflags = disflags | ((uint64_t)1 << dis->disease);
				disflagsprog = disflagsprog | ((uint64_t)1 << dis->disease);
			} else if (dis->status == DiseaseStatus::kRegressing) {
				disflags = disflags | ((uint64_t)1 << dis->disease);
			}
		}
	}
}

void ActorInfo::ResetDiseases()
{
	for (int i = 0; i < Diseases::kMaxValue; i++) {
		if (diseases[i]) {
			LOG1_1("{}[ActorInfo] [Reset] {}", i);
			diseases[i].reset();
		}
	}
	disflags = 0;
	disflagsprog = 0;
}

void ActorInfo::ApplyDiseaseModifier(Diseases::Disease disease, CureDiseaseOption* cure)
{
	if (auto dinfo = FindDisease(disease); dinfo)
	{
		if (dinfo->permanentModifiers & static_cast<EnumType>(cure->modifier))
		{
			// already applied, so apply adjusted strength
			dinfo->permanentModifiersPoints -= cure->strengthadditional;
		}
		else
		{
			// not applied so far, so save application and adjust points
			dinfo->permanentModifiers |= static_cast<EnumType>(cure->modifier);
			dinfo->permanentModifiersPoints -= cure->strengthfirst;
		}
	}
}



#pragma endregion
