#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

#include <xbyak/xbyak.h>

namespace hooks::tweak::vrplayroom
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	static bool isVRPlayroomSettingActive()
	{
		if (!REL::Module::IsVR()) {
			return false;
		}

		return Settings::GetSingleton()->experimental.disableScriptsInPlayroom;
	}

	struct VRPlayroomScriptDisable
	{
		// keep original checks, but also add VRPlayroom check for non-VRPlayroom scripts
		static bool stackCheckIntercept(std::uint32_t a_objectPackedData, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery);

		struct StackCheck : Xbyak::CodeGenerator
		{
			StackCheck(std::uintptr_t jmpIfStackPasses, std::uintptr_t jmpIfStackFails, std::uintptr_t func)
			{
				Xbyak::Label funcLabel;

				mov(rdx, rbx);  // move a_stack into position, rdx will be scratched over after this call
				mov(r8, r12);   // move funcQuery into position, r8 will get scratched over as well
				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				test(al, al);  // rax will be scratched over
				jz("StackFails");
				L("StackPasses");
				mov(rcx, jmpIfStackPasses);  // rcx will be scratched over
				jmp(rcx);
				L("StackFails");
				mov(rcx, jmpIfStackFails);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		// Install our hook at the specified address
		static void Install()
		{
			if (!isVRPlayroomSettingActive()) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(98130, 0x0), REL::Offset(0x1A5) };
			const REL::Relocation stackPasses{ RELOCATION_ID(98130, 0x0), REL::Offset(0x252) };
			const REL::Relocation stackFails{ RELOCATION_ID(98130, 0x0), REL::Offset(0x1C6) };

			auto stackCheckCode = StackCheck(stackPasses.address(), stackFails.address(), reinterpret_cast<uintptr_t>(stackCheckIntercept));
			REL::safe_fill(target.address(), REL::NOP, 0x21);

			auto& trampoline = SKSE::GetTrampoline();
			auto result = trampoline.allocate(stackCheckCode);
			auto& trampoline2 = SKSE::GetTrampoline();
			trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);

			logger::info("VRPlayroomScriptDisable hooked at address {:x}", target.address());
			logger::info("VRPlayroomScriptDisable hooked at offset {:x}", target.offset());
		}
		REGISTER_HOOK(VRPlayroomScriptDisable)
		REGISTER_HOOK_THUNK(1);
		REGISTER_HOOK_CUSTOM_SIZE(0x50);
	};

	struct ReturnToMainMenuHook
	{
		static std::uint64_t thunk(std::uint64_t unk0, std::uint64_t unk1, std::uint64_t unk2, std::uint64_t unk3);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!isVRPlayroomSettingActive()) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(52368, 0x0), REL::Offset(0x38) };

			stl::write_thunk_call<ReturnToMainMenuHook>(target.address());

			logger::info("VRPlayroomScriptDisableMainMenuCallback hooked at address {:x}", target.address());
			logger::info("VRPlayroomScriptDisableMainMenuCallback hooked at offset {:X}", target.offset());
		}
		REGISTER_HOOK(ReturnToMainMenuHook)
		REGISTER_HOOK_THUNK(1)
	};

	struct StackDumpBlockHook
	{
		static void thunk(RE::SkyrimVM* a_vm);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!isVRPlayroomSettingActive()) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(53195, 0x0), REL::Offset(0xEF) };

			stl::write_thunk_call<StackDumpBlockHook>(target.address());

			logger::info("StackDumpBlockHook hooked at address {:x}", target.address());
			logger::info("StackDumpBlockHook hooked at offset {:X}", target.offset());
		}

		REGISTER_HOOK(StackDumpBlockHook)
		REGISTER_HOOK_THUNK(1)
	};

	struct LogStackDumpBlockHook
	{
		static void thunk(std::uint64_t unk0, std::uint64_t unk1, std::uint64_t unk2);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!isVRPlayroomSettingActive()) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(53195, 0x0), REL::Offset(0xE3) };

			stl::write_thunk_call<LogStackDumpBlockHook>(target.address());

			logger::info("LogStackDumpBlockHook hooked at address {:x}", target.address());
			logger::info("LogStackDumpBlockHook hooked at offset {:X}", target.offset());
		}
		REGISTER_HOOK(LogStackDumpBlockHook)
		REGISTER_HOOK_THUNK(1)
	};
}
