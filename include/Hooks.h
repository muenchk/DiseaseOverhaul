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
			REL::Relocation<uintptr_t> target{ REL::VariantID(14497, 14655, 0), REL::VariantOffset(0, 0, 0) }; // needs vr resolution
			auto& trampoline = SKSE::GetTrampoline();

			_GetName = trampoline.write_branch<5>(target.address(), GetName);
		}

	private:
		static RE::BSFixedString GetName(RE::TESForm* form);
		static inline REL::Relocation<decltype(GetName)> _GetName;
	};

	/* class AnimationEventHook
	{
	public:
		static void InstallHook()
		{
			REL::Relocation<uintptr_t> target{ REL::VariantID(36972, 37997, 0), REL::VariantOffset(0x22, 0x22, 0) };
			auto& trampoline = SKSE::GetTrampoline();

			_AnimationEvent = trampoline.write_branch<5>(target.address(), AnimationEvent);
		}

	private:
		static uintptr_t AnimationEvent(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
		static inline REL::Relocation<decltype(AnimationEvent)> _AnimationEvent;
	};*/

	class AnimationEventHook
	{
	public:
		static inline uintptr_t remainder;

		static void Install()
		{
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<uintptr_t> hook1{ REL::RelocationID{ 36972, 37997 }, REL::VariantOffset{ 0x0, 0x0, 0 } }; // needs vr resolution

			struct Patch : Xbyak::CodeGenerator
			{
				Patch(uintptr_t a_func, uintptr_t a_hook)
				{
					Xbyak::Label fdec;

					push(rdi);
					sub(rsp, 0x60);
					call(ptr[rip + fdec]);
					mov(rax, qword[a_func]);
					jmp(rax);

					L(fdec);
					dq(a_hook);
				}
			};

			Patch patch{ (uintptr_t)(&remainder), reinterpret_cast<uintptr_t>(AnimationEvent) };
			patch.ready();

			remainder = trampoline.write_branch<5>(hook1.address(), trampoline.allocate(patch));
			remainder = hook1.address() + 0x6;

			REL::safe_fill(hook1.address() + 0x5, REL::NOP, 1);
		}

	private:
		static void AnimationEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* src);

		static inline REL::Relocation<uintptr_t> _GoFurther;
	};

	class ShowHUDMessageHook
	{
	private:
		static void ShowHUDMessageDecl(const char* message, uint64_t arg_2, bool arg_3);

	public:
		static void ShowHUDMessage(const char* message)
		{
			REL::Relocation<decltype(ShowHUDMessageDecl)> func{ REL::RelocationID{ 52050, 52933 } };
			return func(message, 0, false);
		}
	};

	/// <summary>
	/// executed when confirming fast travel message box on world map
	/// </summary>
	class FastTravelConfirmHook
	{
	public:
		static void InstallHook()
		{
			REL::Relocation<uintptr_t> target{ REL::VariantID(52236, 53127, 0x91a4e0), REL::VariantOffset(0x31, 0x31, 0x31) };
			auto& trampoline = SKSE::GetTrampoline();

			_FastTravelConfirm = trampoline.write_call<5>(target.address(), FastTravelConfirm);
		}

	private:
		static bool FastTravelConfirm(uint64_t self, uint64_t menu);
		static inline REL::Relocation<decltype(FastTravelConfirm)> _FastTravelConfirm;
	};

	class Papyrus_FastTravelHook
	{
	private:
		static inline uint64_t remainder_1;
		static inline uint64_t remainder_2;

	public:
		static void InstallHook()
		{
			REL::Relocation<uintptr_t> targetbegin{ REL::VariantID(54824, 55457, 0), REL::VariantOffset(0x78, 0x78, 0) };
			REL::Relocation<uintptr_t> targetend{ REL::VariantID(54824, 55457, 0), REL::VariantOffset(0xD7, 0xD7, 0) };
			auto& trampoline = SKSE::GetTrampoline();

			struct Patch : Xbyak::CodeGenerator
			{
				Patch(uintptr_t a_remainder, uintptr_t a_fastTravelBegin)
				{
					Xbyak::Label fdec;

					mov(ptr[rsp + 0x50], rcx);
					//mov(rdi, rdx);
					//mov(rsi, rcx);
					call(ptr[rip + fdec]);

					mov(rax, qword[a_remainder]);
					jmp(rax);

					L(fdec);
					dq(a_fastTravelBegin);
				}
			};

			Patch patch{ (uintptr_t)(&remainder_1), reinterpret_cast<uintptr_t>(FastTravelBegin) };
			patch.ready();

			remainder_1 = trampoline.write_branch<5>(targetbegin.address(), trampoline.allocate(patch));
			remainder_1 = targetbegin.address() + 0x5;
		}

	private:
		static void FastTravelBegin();
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
