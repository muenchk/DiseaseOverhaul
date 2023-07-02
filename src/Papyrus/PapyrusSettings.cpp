
#include "Data.h"
#include "Papyrus/PapyrusSettings.h"
#include "Settings.h"

namespace Papyrus
{
	namespace PSettings
	{

		int GetSystem_CycleTime(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::System::_cycletime;
		}
		void SetSystem_CycleTime(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, int value)
		{
			Settings::System::_cycletime = value;
		}
		float GetSystem_TickLength(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::System::_ticklength;
		}
		void SetSystem_TickLength(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, float value)
		{
			Settings::System::_ticklength = value;
		}
		bool GetSystem_ShowDiseaseEffects(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::System::_showDiseaseEffects;
		}
		void SetSystem_ShowDiseaseEffects(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::System::_showDiseaseEffects = value;
		}
		float GetSystem_TickLengthInfectionRegression(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::System::_ticklengthInfectionRegression;
		}
		void SetSystem_TickLengthInfectionRegression(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, float value)
		{
			Settings::System::_ticklengthInfectionRegression = value;
		}
		bool GetDebug_EnableLog(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Debug::EnableLog;
		}
		void SetDebug_EnableLog(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Debug::EnableLog = value;
			Logging::EnableLog = value;
		}
		bool GetDebug_EnableLoadLog(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Debug::EnableLoadLog;
		}
		void SetDebug_EnableLoadLog(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Debug::EnableLoadLog = value;
			Logging::EnableLoadLog = value;
		}
		int GetDebug_LogLevel(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Debug::LogLevel;
		}
		void SetDebug_LogLevel(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, int value)
		{
			Settings::Debug::LogLevel = value;
			Logging::LogLevel = value;
		}
		bool GetDebug_EnableProfiling(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Debug::EnableProfiling;
		}
		void SetDebug_EnableProfiling(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Debug::EnableProfiling = value;
			Logging::EnableProfiling = value;
		}
		int GetDebug_ProfileLevel(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Debug::ProfileLevel;
		}
		void SetDebug_ProfileLevel(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, int value)
		{
			Settings::Debug::ProfileLevel = value;
			Logging::ProfileLevel = value;
		}
		bool GetDisease_IgnoreVampireBaseImmunity(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Disease::_ignoreVampireBaseImmunity;
		}
		void SetDisease_IgnoreVampireBaseImmunity(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Disease::_ignoreVampireBaseImmunity = value;
		}
		bool GetDisease_IgnoreWerewolfBaseImmunity(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Disease::_ignoreWerewolfBaseImmunity;
		}
		void SetDisease_IgnoreWerewolfBaseImmunity(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Disease::_ignoreWerewolfBaseImmunity = value;
		}
		bool GetDisease_IgnoreDiseaseResistance(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Disease::_ignoreDiseaseResistance;
		}
		void SetDisease_IgnoreDiseaseResistance(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Disease::_ignoreDiseaseResistance = value;
		}
		float GetDisease_IgnoreTimeAdvancementConstraints(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Disease::_ignoreTimeAdvancementConstraint;
		}
		void SetDisease_IgnoreTimeAdvancementConstraints(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Disease::_ignoreTimeAdvancementConstraint = value;
		}
		bool GetDisease_AllowActorDeath(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Disease::_AllowActorDeath;
		}
		void SetDisease_AllowActorDeath(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Disease::_AllowActorDeath = value;
		}
		int GetInfection_ParticleRange(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Disease::_particleRange;
		}
		void SetInfection_ParticleRange(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Disease::_particleRange = value;
		}
		bool GetInfection_AllowMultipleDiseases(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Infection::_AllowMultipleDiseases;
		}
		void SetInfection_AllowMultipleDiseases(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Infection::_AllowMultipleDiseases = value;
		}
		bool GetInfection_ShowInfectionStatus(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*)
		{
			return Settings::Infection::_ShowInfectionStatus;
		}
		void SetInfection_ShowInfectionStatus(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, bool value)
		{
			Settings::Infection::_ShowInfectionStatus = value;
		}

		bool Register(RE::BSScript::Internal::VirtualMachine* a_vm)
		{
			a_vm->RegisterFunction(std::string("GetSystem_CycleTime"), script, GetSystem_CycleTime);
			a_vm->RegisterFunction(std::string("SetSystem_CycleTime"), script, SetSystem_CycleTime);
			a_vm->RegisterFunction(std::string("GetSystem_TickLength"), script, GetSystem_TickLength);
			a_vm->RegisterFunction(std::string("SetSystem_TickLength"), script, SetSystem_TickLength);
			a_vm->RegisterFunction(std::string("GetSystem_ShowDiseaseEffects"), script, GetSystem_ShowDiseaseEffects);
			a_vm->RegisterFunction(std::string("SetSystem_ShowDiseaseEffects"), script, SetSystem_ShowDiseaseEffects);
			a_vm->RegisterFunction(std::string("GetSystem_TickLengthInfectionRegression"), script, GetSystem_TickLengthInfectionRegression);
			a_vm->RegisterFunction(std::string("SetSystem_TickLengthInfectionRegression"), script, SetSystem_TickLengthInfectionRegression);
			a_vm->RegisterFunction(std::string("GetDebug_EnableLog"), script, GetDebug_EnableLog);
			a_vm->RegisterFunction(std::string("SetDebug_EnableLog"), script, SetDebug_EnableLog);
			a_vm->RegisterFunction(std::string("GetDebug_EnableLoadLog"), script, GetDebug_EnableLoadLog);
			a_vm->RegisterFunction(std::string("SetDebug_EnableLoadLog"), script, SetDebug_EnableLoadLog);
			a_vm->RegisterFunction(std::string("GetDebug_LogLevel"), script, GetDebug_LogLevel);
			a_vm->RegisterFunction(std::string("SetDebug_LogLevel"), script, SetDebug_LogLevel);
			a_vm->RegisterFunction(std::string("GetDebug_EnableProfiling"), script, GetDebug_EnableProfiling);
			a_vm->RegisterFunction(std::string("SetDebug_EnableProfiling"), script, SetDebug_EnableProfiling);
			a_vm->RegisterFunction(std::string("GetDebug_ProfileLevel"), script, GetDebug_ProfileLevel);
			a_vm->RegisterFunction(std::string("SetDebug_ProfileLevel"), script, SetDebug_ProfileLevel);
			a_vm->RegisterFunction(std::string("GetDisease_IgnoreVampireBaseImmunity"), script, GetDisease_IgnoreVampireBaseImmunity);
			a_vm->RegisterFunction(std::string("SetDisease_IgnoreVampireBaseImmunity"), script, SetDisease_IgnoreVampireBaseImmunity);
			a_vm->RegisterFunction(std::string("GetDisease_IgnoreWerewolfBaseImmunity"), script, GetDisease_IgnoreWerewolfBaseImmunity);
			a_vm->RegisterFunction(std::string("SetDisease_IgnoreWerewolfBaseImmunity"), script, SetDisease_IgnoreWerewolfBaseImmunity);
			a_vm->RegisterFunction(std::string("GetDisease_IgnoreDiseaseResistance"), script, GetDisease_IgnoreDiseaseResistance);
			a_vm->RegisterFunction(std::string("SetDisease_IgnoreDiseaseResistance"), script, SetDisease_IgnoreDiseaseResistance);
			a_vm->RegisterFunction(std::string("GetDisease_IgnoreTimeAdvancementConstraints"), script, GetDisease_IgnoreTimeAdvancementConstraints);
			a_vm->RegisterFunction(std::string("SetDisease_IgnoreTimeAdvancementConstraints"), script, SetDisease_IgnoreTimeAdvancementConstraints);
			a_vm->RegisterFunction(std::string("GetDisease_AllowActorDeath"), script, GetDisease_AllowActorDeath);
			a_vm->RegisterFunction(std::string("SetDisease_AllowActorDeath"), script, SetDisease_AllowActorDeath);
			a_vm->RegisterFunction(std::string("GetInfection_ParticleRange"), script, GetInfection_ParticleRange);
			a_vm->RegisterFunction(std::string("SetInfection_ParticleRange"), script, SetInfection_ParticleRange);
			a_vm->RegisterFunction(std::string("GetInfection_AllowMultipleDiseases"), script, GetInfection_AllowMultipleDiseases);
			a_vm->RegisterFunction(std::string("SetInfection_AllowMultipleDiseases"), script, SetInfection_AllowMultipleDiseases);
			a_vm->RegisterFunction(std::string("GetInfection_ShowInfectionStatus"), script, GetInfection_ShowInfectionStatus);
			a_vm->RegisterFunction(std::string("SetInfection_ShowInfectionStatus"), script, SetInfection_ShowInfectionStatus);

			return true;
		}
	}
}
