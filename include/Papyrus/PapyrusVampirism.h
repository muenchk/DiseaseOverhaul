#pragma once

namespace Papyrus
{
	namespace Vampirism
	{
		const std::string script = "DiseaseOverhaul_Infections";

		void InfectSanguinareVampirism(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, RE::Actor* actor);

		bool Register(RE::BSScript::Internal::VirtualMachine* a_vm);
	}
}
