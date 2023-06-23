#include "DiseaseStats.h"
#include "Data.h"
#include "Random.h"
#include "Logging.h"
#include "Stats.h"
#include "Settings.h"

namespace DisSta
{
	Data* data = nullptr;
}

DiseaseInfo* DiseaseStats::FindDisease(Diseases::Disease value)
{
	for (int i = 0; i < diseases.size(); i++) {
		if (diseases[i] && diseases[i]->disease == value)
			return diseases[i];
	}
	return nullptr;
}


bool DiseaseStats::ProgressDisease(RE::Actor* actor, Diseases::Disease value, float points)
{
	if (actor == nullptr)
		return false;

	LOG_3("{}[DiseaseStats] [ProgressDisease]");

	// stat tracking
	Stats::DiseaseStats_ProgressDisease++;

	// init data if not already happened
	if (DisSta::data == nullptr)
		DisSta::data = Data::GetSingleton();

	bool killac = false;

	// if the points are 0 just return
	if (points == 0)
		return false;

	DiseaseInfo* dinfo = FindDisease(value);
	if (dinfo == nullptr) {
		// if there is no disease info and we want to loose points, then there is no meaning in creating info at all
		if (points < 0)
			return false;
		dinfo = new DiseaseInfo();
		dinfo->advPoints = 0;
		dinfo->disease = value;
		dinfo->earliestAdvancement = 0;
		dinfo->immuneUntil = 0;
		dinfo->permanentModifiers = 0;
		dinfo->permanentModifiersPoints = 0;
		dinfo->stage = 0;
		dinfo->status = DiseaseStatus::kInfection;
		diseases.push_back(dinfo);
	}

	float currentgameday = RE::Calendar::GetSingleton()->GetDaysPassed();

	// check for immunity, if immune don't add points, just deduct
	if (dinfo->immuneUntil > currentgameday)
		return false;

	LOG_4("{}[DiseaseStats] [ProgressDisease] start work");

	RE::ActorHandle achandle = actor->GetHandle();

	// do actual work
	Disease* dis = DisSta::data->GetDisease(value);
	dinfo->advPoints += points;
	switch (dinfo->status) {
	case DiseaseStatus::kInfection:
		LOG_4("{}[DiseaseStats] [ProgressDisease] Infection");
		// infection stage means no effects
		// we have to check whether we jump out of infection stage and calculate effects
		if (dinfo->advPoints > dis->_stageInfection->_advancementThreshold) {
			LOG_4("{}[DiseaseStats] [ProgressDisease] advance");
			// advance to incubation
			dinfo->stage = 0;
			dinfo->status = DiseaseStatus::kProgressing;
			// carry over points up to half of the next stage
			dinfo->advPoints -= dis->_stageInfection->_advancementThreshold; // carry over excessive infection points
			if (dinfo->advPoints > (dis->_stages[0]->_advancementThreshold / 2))
				dinfo->advPoints = (float)dis->_stages[0]->_advancementThreshold / 2;
			dinfo->earliestAdvancement = currentgameday + dis->_stages[0]->_advancementTime;
			dinfo->permanentModifiers = 0;
			dinfo->permanentModifiersPoints = 0;
			// apply effect if there is one
			if (dis->_stages[0]->effect != nullptr)
				actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[0]->effect, true, actor, 100, false, false, actor);
		} else if (dinfo->advPoints <= 0) {
			LOG_4("{}[DiseaseStats] [ProgressDisease] delete info");
			// we had a regressing effect, so go down stages
			// in essence we don't have an infection anymore, so remove dieseaseinfo
			auto itr = diseases.begin();
			while (itr != diseases.end()) {
				if ((*itr)->disease == value) {
						diseases.erase(itr);
						delete dinfo;
						return false; // go straight out since we are done
					}
				itr++;
			}
		}
		break;
	case DiseaseStatus::kProgressing:
		LOG_4("{}[DiseaseStats] [ProgressDisease] Progressing");
		// this is more tricky, first check current stage and then advancement
		if (dinfo->advPoints > dis->_stages[dinfo->stage]->_advancementThreshold) {
			// try advance stage
			// if we have reached the maximum stage, calculate endeffects set return value to true if actor should die and then calculate rest as normal
			if (dinfo->stage == dis->_numstages) {
				LOG_4("{}[DiseaseStats] [ProgressDisease] max stage exceeded");
				// calculate end effects

				// apply end effect
				if (dis->endeffect != nullptr)
					actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->endeffect, true, actor, 100, false, false, actor);
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
				LOG_4("{}[DiseaseStats] [ProgressDisease] advance");
				// advance to next stage
				// remove last effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					actor->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
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
					actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
			}
			// else dont advance just accumulate points until advancement
		} else if (dinfo->advPoints < 0) {
			// we had a regressing effect so go down stages
			if (dinfo->stage <= 1) { // incubation stage does not exist for regression
				LOG_4("{}[DiseaseStats] [ProgressDisease] gain immunity");
				// we are already in the lowest stage, so go back to infection and gain immunity
				dinfo->status = DiseaseStatus::kInfection;
				dinfo->earliestAdvancement = 0;
				dinfo->immuneUntil = currentgameday + dis->immunityTime;
				dinfo->permanentModifiers = 0;
				dinfo->permanentModifiersPoints = 0;
				// remove effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					actor->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
			} else {
				LOG_4("{}[DiseaseStats] [ProgressDisease] devance");
				// we just regress one stage
				// remove last effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					actor->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
				dinfo->stage--;
				dinfo->advPoints = (float)dis->_stages[dinfo->stage]->_advancementThreshold;
				// apply effect if there is one
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
			}
		}
		break;
	case DiseaseStatus::kRegressing:
		LOG_4("{}[DiseaseStats] [ProgressDisease] Regressing");
		// if you are regressing you cannot progress anymore, even if you were to gain advPoints
		if (dinfo->advPoints < 0) {
			// we had a regressing effect so go down stages
			if (dinfo->stage <= 1) {  // incubation stage does not exist for regression
				LOG_4("{}[DiseaseStats] [ProgressDisease] gain immunity");
				// we are already in the lowest stage, so go back to infection and gain immunity
				dinfo->status = DiseaseStatus::kInfection;
				dinfo->earliestAdvancement = 0;
				dinfo->immuneUntil = currentgameday + dis->immunityTime;
				dinfo->permanentModifiers = 0;
				dinfo->permanentModifiersPoints = 0;
				// remove effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					actor->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
			} else {
				LOG_4("{}[DiseaseStats] [ProgressDisease] devance");
				// we just regress one stage
				// remove last effect
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					actor->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
				dinfo->stage--;
				dinfo->advPoints = (float)dis->_stages[dinfo->stage]->_advancementThreshold;
				// apply effect if there is one
				if (dis->_stages[dinfo->stage]->effect != nullptr)
					actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
			}
		}
		break;
	}

	LOG_4("{}[DiseaseStats] [ProgressDisease] end");
	return killac;
}

bool DiseaseStats::ForceIncreaseStage(RE::Actor* actor, Diseases::Disease value)
{
	LOG_2("{}[DiseaseStats] [ForceIncreaseStage]");
	if (DisSta::data == nullptr)
		DisSta::data = Data::GetSingleton();

	bool killac = false;

	DiseaseInfo* dinfo = FindDisease(value);
	if (dinfo == nullptr) {
		dinfo = new DiseaseInfo();
		dinfo->advPoints = 0;
		dinfo->disease = value;
		dinfo->earliestAdvancement = 0;
		dinfo->immuneUntil = 0;
		dinfo->permanentModifiers = 0;
		dinfo->permanentModifiersPoints = 0;
		dinfo->stage = 0;
		dinfo->status = DiseaseStatus::kInfection;
		diseases.push_back(dinfo);
	}

	float currentgameday = RE::Calendar::GetSingleton()->GetDaysPassed();

	RE::ActorHandle achandle = actor->GetHandle();

	// do actual work
	Disease* dis = DisSta::data->GetDisease(value);
	switch (dinfo->status) {
	case DiseaseStatus::kInfection:
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] Infection");
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] advance");
		// advance to incubation
		dinfo->stage = 0;
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] 1");
		dinfo->status = DiseaseStatus::kProgressing;
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] 2");
		dinfo->advPoints = 0;
		LOG1_4("{}[DiseaseStats] [ForceIncreaseStage] 3 {}", (uint64_t)dis->_stages);
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] 4");
		dinfo->earliestAdvancement = currentgameday + dis->_stages[0]->_advancementTime;
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] 5");
		dinfo->permanentModifiers = 0;
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] 6");
		dinfo->permanentModifiersPoints = 0;
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] 7");
		// apply effect if there is one
		if (dis->_stages[0]->effect != nullptr)
			actor->AddSpell(dis->_stages[0]->effect);
			//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[0]->effect, true, actor, 100, false, false, actor);
			//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->SpellCast(false, 0, dis->_stages[0]->effect);
		else
			LOG_4("{}[DiseaseStats] [ForceIncreaseStage] the stage does not have any effect");
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] 8");
		break;
	case DiseaseStatus::kProgressing:
	case DiseaseStatus::kRegressing:
		LOG_4("{}[DiseaseStats] [ForceIncreaseStage] Gressing");
		if (dinfo->stage == dis->_numstages) {
			LOG_4("{}[DiseaseStats] [ForceIncreaseStage] max stage exceeded");
			// calculate end effects

			// apply end effect
			if (dis->endeffect != nullptr)
				actor->AddSpell(dis->endeffect);
				//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->endeffect, true, actor, 100, false, false, actor);
				//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->SpellCast(false, 0, dis->endeffect);
			else
				LOG_4("{}[DiseaseStats] [ForceIncreaseStage] there is no endeffect");
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
			LOG_4("{}[DiseaseStats] [ForceIncreaseStage] advance");
			// advance to next stage
			// remove last effect
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				actor->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
			dinfo->advPoints = 0;
			dinfo->stage++;
			dinfo->earliestAdvancement = currentgameday + dis->_stages[dinfo->stage]->_advancementTime;
			// apply effect if there is one
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				actor->AddSpell(dis->_stages[dinfo->stage]->effect);
				//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
				//actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->SpellCast(false, 0, dis->_stages[dinfo->stage]->effect);
			else
				LOG_4("{}[DiseaseStats] [ForceIncreaseStage] the stage does not have any effect");
		}
		break;
	}
	LOG_4("{}[DiseaseStats] [ForceIncreaseStage] end");
	return killac;
}

void DiseaseStats::ForceDecreaseStage(RE::Actor* actor, Diseases::Disease value)
{
	LOG_2("{}[DiseaseStats] [ForceDecreaseStage]");
	if (DisSta::data == nullptr)
		DisSta::data = Data::GetSingleton();

	bool killac = false;

	DiseaseInfo* dinfo = FindDisease(value);
	if (dinfo == nullptr) {
		dinfo = new DiseaseInfo();
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

	RE::ActorHandle achandle = actor->GetHandle();

	// do actual work
	Disease* dis = DisSta::data->GetDisease(value);
	switch (dinfo->status) {
	case DiseaseStatus::kInfection:
		LOG_4("{}[DiseaseStats] [ForceDecreaseStage] Infection");
		LOG_4("{}[DiseaseStats] [ForceDecreaseStage] delete info");
		{
			// we had a regressing effect, so go down stages
			// in essence we don't have an infection anymore, so remove dieseaseinfo
			auto itr = diseases.begin();
			while (itr != diseases.end()) {
				if ((*itr)->disease == value) {
					{
						diseases.erase(itr);
						delete dinfo;
						return;  // go straight out since we are done
					}
					itr++;
				}
			}
		}
		break;
	case DiseaseStatus::kProgressing:
	case DiseaseStatus::kRegressing:
		LOG_4("{}[DiseaseStats] [ForceDecreaseStage] Gressing");
		if (dinfo->stage <= 1) {  // incubation stage does not exist for regression
			LOG_4("{}[DiseaseStats] [ForceDecreaseStage] gain immunity");
			// we are already in the lowest stage, so go back to infection and gain immunity
			dinfo->status = DiseaseStatus::kInfection;
			dinfo->earliestAdvancement = 0;
			dinfo->immuneUntil = currentgameday + dis->immunityTime;
			dinfo->permanentModifiers = 0;
			dinfo->permanentModifiersPoints = 0;
			// remove effect
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				actor->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
		} else {
			LOG_4("{}[DiseaseStats] [ForceDecreaseStage] devance");
			// we just regress one stage
			// remove last effect
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				actor->DispelEffect(dis->_stages[dinfo->stage]->effect, achandle, nullptr);
			dinfo->stage--;
			dinfo->advPoints = (float)dis->_stages[dinfo->stage]->_advancementThreshold;
			// apply effect if there is one
			if (dis->_stages[dinfo->stage]->effect != nullptr)
				actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(dis->_stages[dinfo->stage]->effect, true, actor, 100, false, false, actor);
		}
		break;
	}
	LOG_4("{}[DiseaseStats] [ForceDecreaseStage] end");
}

void DiseaseStats::CleanDiseases()
{
	auto itr = diseases.begin();
	DiseaseInfo* dinfo = nullptr;
	while (itr != diseases.end())
	{
		dinfo = *itr;
		if (dinfo->status == DiseaseStatus::kInfection && RE::Calendar::GetSingleton()->GetDaysPassed() > dinfo->immuneUntil && dinfo->advPoints == 0) {
			delete dinfo;
			diseases.erase(itr);
		}
		itr++;
	}
}

void DiseaseStats::Reset()
{
	for (int i = 0; i < diseases.size(); i++) {
		delete diseases[i];
	}
	diseases.clear();
}
