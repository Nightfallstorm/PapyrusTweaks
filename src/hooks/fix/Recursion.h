#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::fix::recursion
{
	struct StackOverFlowHook
	{
		static RE::BSFixedString* thunk(std::uint64_t unk0, RE::BSScript::Stack* a_stack, std::uint64_t* a_funcCallQuery);

		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			if (!Settings::GetSingleton()->fixes.fixRecursionOverflow) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x7F, 0x7F, 0x7F) };
			stl::write_thunk_call<StackOverFlowHook>(target.address());

			logger::info("StackFrameOverFlow hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("StackFrameOverFlow hooked at offset {}", fmt::format("{:x}", target.offset()));
		}

		REGISTER_HOOK(StackOverFlowHook)
		REGISTER_HOOK_THUNK(1)
	};

	struct StackOverFlowLogHook
	{
		static void thunk(RE::BSScript::Stack* a_stack, const char* a_source, std::uint32_t unk2, char* unk3, std::uint32_t sizeInBytes);

		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			if (!Settings::GetSingleton()->fixes.fixRecursionOverflow) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x963, 0x97A, 0x963) };
			stl::write_thunk_call<StackOverFlowLogHook>(target.address());

			logger::info("StackFrameOverFlowLog hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("StackFrameOverFlowLog hooked at offset {}", fmt::format("{:x}", target.offset()));
		}

		REGISTER_HOOK(StackOverFlowLogHook)
		REGISTER_HOOK_THUNK(1)
	};
}