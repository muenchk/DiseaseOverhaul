#pragma once

namespace Papyrus
{
	namespace PUtility
	{
		const std::string script = "DiseaseOverhaul_Utility";

		void RemoveAllDiseases(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, RE::Actor* actor);

		bool Register(RE::BSScript::Internal::VirtualMachine* a_vm);
	}
}
