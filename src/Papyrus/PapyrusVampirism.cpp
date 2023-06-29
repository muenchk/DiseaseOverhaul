

#include "Data.h"
#include "Papyrus/PapyrusVampirism.h"


namespace Papyrus
{
	namespace Vampirism
	{

		void InfectSanguinareVampirism(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, RE::Actor* actor)
		{
			if (actor != nullptr)
			{
				auto acinfo = Data::GetSingleton()->FindActor(actor);
				if (acinfo->IsValid())
				{
					if (acinfo->IsInfected(Diseases::Disease::kSanguinareVampirism) == false) {
						// Skip directly to first stage
						static_cast<void>(acinfo->ForceIncreaseStage(Diseases::Disease::kSanguinareVampirism));
						static_cast<void>(acinfo->ForceIncreaseStage(Diseases::Disease::kSanguinareVampirism));
					}
					else
					{
						static_cast<void>(acinfo->ProgressDisease(Diseases::Disease::kSanguinareVampirism, 50));
					}
				}
			}
		}


		bool Register(RE::BSScript::Internal::VirtualMachine* a_vm)
		{
			a_vm->RegisterFunction(std::string("InfectSanguinareVampirism"), script, InfectSanguinareVampirism);

			return true;
		}
	}
}
