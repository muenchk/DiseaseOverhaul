#include <Disease.h>


Disease::Spreadingbase Disease::CalcPossibleInfectionConditions(uint32_t cellid)
{
	TESForm* form = RE::TESForm::LookupByID(cellid);
	if (form) {
		RE::TESObjectCELL* cell = form->As<RE::TESObjectCELL>();
		if (cell) {
			// we have the cell now check for the conditions
		}
	}
}
