#pragma once

namespace Hooks
{
	/// <summary>
	/// executed when fading to fast travel (from world map, not carriage)
	/// </summary>
	class GetNameHook
	{
	public:
		static void InstallHook()
		{
			REL::Relocation<uintptr_t> target{ REL::VariantID(14497, 14655, 0), REL::VariantOffset(0, 0, 0) };
			auto& trampoline = SKSE::GetTrampoline();

			_GetName = trampoline.write_branch<5>(target.address(), GetName);
		}

	private:
		static RE::BSFixedString GetName(RE::TESForm* form);
		static inline REL::Relocation<decltype(GetName)> _GetName;
	};

	void InstallHooks();
}










namespace Hooks_Funcs
{
	const char* GetName(RE::TESObjectREFR* obj, const char* origName);
}


// The following is taken from PowrerOfThree's NPCs-Names-Distrbutor, on 26/06/2023 under the MIT license
// Credits to him for the hooks location and modifications
// the function code has been modified to this mods needs

namespace PowerOfThree
{
	/// Vanilla: Full.
	///	    NND: Full.
	/// Name displayed in all other cases, like notifications.
	struct GetDisplayFullName_GetDisplayName
	{
		static const char* thunk(RE::ExtraTextDisplayData* a_this, RE::TESObjectREFR* obj, float temperFactor)
		{
			const auto originalName = a_this->GetDisplayName(obj->GetBaseObject(), temperFactor);
			return Hooks_Funcs::GetName(obj, originalName);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	/// Vanilla: Full.
	///	    NND: Full.
	/// Name displayed in all other cases, like notifications.
	struct GetDisplayFullName_GetFormName
	{
		static const char* thunk(RE::TESObjectREFR* a_this)
		{
			if (const auto base = a_this->GetBaseObject(); base) {
				const auto originalName = base->GetName();
				return Hooks_Funcs::GetName(a_this, originalName);
			}
			// Fallback for cases when GetBaseObject is nullptr.
			// According to crash reports this might happen when a_this is being deleted by the game?
			// It has Flag kDeleted and objectReference is nullptr, so I guess this is it.
			return a_this->GetDisplayFullName();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	inline void Install()
	{
		const REL::Relocation<std::uintptr_t> displayFullName{ RE::Offset::TESObjectREFR::GetDisplayFullName };

		auto& trampoline = SKSE::GetTrampoline();

		// Swaps the argument in TESForm::GetFormName_1401A38F0(object->data.objectReference) to pass TESObjectREFR* obj instead of obj->GetBaseObject()
		// mov rcx, [r15+40h] (49 8B 4F 40)
		//                     v  v  v  +
		// mov rcx, r15       (4C 89 F9) + 90
		REL::safe_fill(displayFullName.address() + REL::VariantOffset{ 0x232, 0x228, 0 }.offset(), 0x4C, 1);
		REL::safe_fill(displayFullName.address() + REL::VariantOffset{ 0x233, 0x229, 0 }.offset(), 0x89, 1);
		REL::safe_fill(displayFullName.address() + REL::VariantOffset{ 0x234, 0x22A, 0 }.offset(), 0xF9, 1);
		REL::safe_fill(displayFullName.address() + REL::VariantOffset{ 0x235, 0x22B, 0 }.offset(), 0x90, 1);
		GetDisplayFullName_GetFormName::func = trampoline.write_call<5>(displayFullName.address() + REL::VariantOffset{ 0x236, 0x22C, 0 }.offset(), GetDisplayFullName_GetFormName::thunk);

		// Swaps 2nd argument in ExtraTextDisplayData::GetDisplayName_140145820(extraTextData, object->data.objectReference, temperFactor) to pass TESObjectREFR* obj instead of obj->GetBaseObject()
		// mov rdx, [r15+40h] (49 8B 57 40)
		//                     v  v  v   +
		// mov rdx, r15       (4C 89 FA) + 90
		REL::safe_fill(displayFullName.address() + REL::VariantOffset{ 0x240, 0x236, 0 }.offset(), 0x4C, 1);
		REL::safe_fill(displayFullName.address() + REL::VariantOffset{ 0x241, 0x237, 0 }.offset(), 0x89, 1);
		REL::safe_fill(displayFullName.address() + REL::VariantOffset{ 0x242, 0x238, 0 }.offset(), 0xFA, 1);
		REL::safe_fill(displayFullName.address() + REL::VariantOffset{ 0x243, 0x239, 0 }.offset(), 0x90, 1);
		GetDisplayFullName_GetDisplayName::func = trampoline.write_call<5>(displayFullName.address() + REL::VariantOffset{ 0x247, 0x23D, 0 }.offset(), GetDisplayFullName_GetDisplayName::thunk);

		logger::info("Installed Default GetDisplayFullName hooks");
	}
}
