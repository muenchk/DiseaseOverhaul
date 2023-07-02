

#include "Data.h"
#include "Papyrus/PapyrusUtility.h"

namespace Papyrus
{
	namespace PUtility
	{

		void RemoveAllDiseases(RE::BSScript::Internal::VirtualMachine* /*a_vm*/, RE::VMStackID /*a_stackID*/, RE::StaticFunctionTag*, RE::Actor* actor)
		{
			// if there is an actor given, remove all diseases from this actor
			if (actor != nullptr) {
				auto acinfo = Data::GetSingleton()->FindActor(actor);
				if (acinfo->IsValid()) {
					acinfo->RemoveAllDiseases();
				}
			} else {
				// otherwise we will have to remove all diseases from all actors
				Data::GetSingleton()->RemoveAllDiseases();
			}
		}

		bool Register(RE::BSScript::Internal::VirtualMachine* a_vm)
		{
			a_vm->RegisterFunction(std::string("RemoveAllDiseases"), script, RemoveAllDiseases);

			return true;
		}
	}
}
