#include "Disease.h"
#include "Logging.h"


Disease::Spreadingbase Disease::CalcPossibleInfectionConditions(uint32_t cellid)
{
	LOG_3("{}[Disease] [CalcPossibleInfectionConditions]");
	RE::TESForm* form = RE::TESForm::LookupByID(cellid);
	if (form) {
		RE::TESObjectCELL* cell = form->As<RE::TESObjectCELL>();
		if (cell) {
			// we have the cell now check for the conditions
		}
	}
}
