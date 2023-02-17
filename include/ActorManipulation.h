#pragma once
#include <chrono>
#include <time.h>
#include <random>
#include <stdlib.h>

#include "Settings.h"
#include "UtilityAlch.h"
#include "Data.h"


/// <summary>
/// provides actor related functions
/// </summary>
class ACM
{
public:

	#pragma region AVFunctions
	/// <summary>
	/// Returns the current maximum for an actor value.
	/// This takes the base av and any modifiers into account
	/// </summary>
	/// <param name="_actor"></param>
	/// <param name="av"></param>
	/// <returns></returns>
	static float GetAVMax(RE::Actor* _actor, RE::ActorValue av)
	{
		// add base value, permanent modifiers and temporary modifiers (magic effects for instance)
		return _actor->GetPermanentActorValue(av) + _actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, av);
	}
	/// <summary>
	/// Returns the current percentage of an actor value (like percentag of health remaining)
	/// </summary>
	/// <param name="_actor">Actor to get av from</param>
	/// <param name="av">Actor value to compute</param>
	/// <returns></returns>
	static float GetAVPercentage(RE::Actor* _actor, RE::ActorValue av)
	{
		return _actor->GetActorValue(av) / (_actor->GetPermanentActorValue(av) + _actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, av));
		/*float tmp = _actor->GetActorValue(av) / (_actor->GetPermanentActorValue(av) + _actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, av));
		logger::info("[GetAVPercentage] {}", tmp);
		return tmp;*/
	}
	/// <summary>
	/// Returns the actor value percentage of an actor calculated from their base value
	/// and a given current value
	/// </summary>
	/// <param name="_actor">Actor to get av from</param>
	/// <param name="av">Actor value to compute</param>
	/// <param name="curr">current value</param>
	/// <returns></returns>
	static float GetAVPercentageFromValue(RE::Actor* _actor, RE::ActorValue av, float curr)
	{
		return curr / (_actor->GetPermanentActorValue(av) + _actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, av));
		/*float tmp = curr / (_actor->GetPermanentActorValue(av) + _actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, av));
		logger::info("[GetAVPercentageFromValue] {}", tmp);
		return tmp;*/
	}
	#pragma endregion

};
