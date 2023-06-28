#include "Logging.h"
#include "Papyrus.h"
#include "Papyrus/PapyrusVampirism.h"

namespace Papyrus
{
	RE::BSScript::Internal::VirtualMachine* VM = nullptr;

	bool Register(RE::BSScript::Internal::VirtualMachine* a_vm)
	{
		if (!a_vm) {
			logwarn("[Papyrus] [Register] VM not available");
			return false;
		}

		VM = a_vm;

		Vampirism::Register(a_vm);

		LOG_1("{}[Papyrus] [Register] Papyrus functions have been registered.");
		return true;
	}
}
