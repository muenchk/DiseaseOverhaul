#pragma once

namespace Papyrus
{
	namespace PSettings
	{
		const std::string script = "DiseaseOverhaul_Settings";

		int GetSystem_CycleTime(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetSystem_CycleTime(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, int value);
		float GetSystem_TickLength(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetSystem_TickLength(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, float value);
		bool GetSystem_ShowDiseaseEffects(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetSystem_ShowDiseaseEffects(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		float GetSystem_TickLengthInfectionRegression(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetSystem_TickLengthInfectionRegression(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, float value);
		bool GetDebug_EnableLog(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDebug_EnableLog(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		bool GetDebug_EnableLoadLog(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDebug_EnableLoadLog(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		int GetDebug_LogLevel(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDebug_LogLevel(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, int value);
		bool GetDebug_EnableProfiling(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDebug_EnableProfiling(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		int GetDebug_ProfileLevel(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDebug_ProfileLevel(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, int value);
		bool GetDisease_IgnoreVampireBaseImmunity(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDisease_IgnoreVampireBaseImmunity(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		bool GetDisease_IgnoreWerewolfBaseImmunity(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDisease_IgnoreWerewolfBaseImmunity(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		bool GetDisease_IgnoreDiseaseResistance(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDisease_IgnoreDiseaseResistance(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		float GetDisease_IgnoreTimeAdvancementConstraints(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDisease_IgnoreTimeAdvancementConstraints(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		bool GetDisease_AllowActorDeath(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetDisease_AllowActorDeath(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		int GetInfection_ParticleRange(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetInfection_ParticleRange(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		bool GetInfection_AllowMultipleDiseases(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetInfection_AllowMultipleDiseases(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);
		bool GetInfection_ShowInfectionStatus(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*);
		void SetInfection_ShowInfectionStatus(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value);

		bool Register(RE::BSScript::Internal::VirtualMachine* a_vm);
	}
}
