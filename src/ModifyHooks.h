#pragma once
#include "Settings.h"
#include <xbyak/xbyak.h>

// TEMPORARY UNTIL CLIB-NG UPDATES
namespace RE::BSScript::UnlinkedTypes
{
	class ConvertTypeFunctor
	{
	public:
		inline static constexpr auto RTTI = RTTI_BSScript__UnlinkedTypes__Function__ConvertTypeFunctor;
		inline static constexpr auto VTABLE = VTABLE_BSScript__UnlinkedTypes__Function__ConvertTypeFunctor;

		virtual ~ConvertTypeFunctor();  // 00

		virtual bool ConvertVariableType(BSFixedString* a_typeAsString, TypeInfo& a_typeOut) = 0;  // 01
	};
	static_assert(sizeof(ConvertTypeFunctor) == 0x8);

	class LinkerConvertTypeFunctor : public ConvertTypeFunctor
	{
	public:
		inline static constexpr auto RTTI = RTTI_BSScript____LinkerConvertTypeFunctor;
		inline static constexpr auto VTABLE = VTABLE_BSScript____LinkerConvertTypeFunctor;
		~LinkerConvertTypeFunctor() override;  // 00

		bool ConvertVariableType(BSFixedString* a_typeAsString, TypeInfo& a_typeOut) override;  // 01
		// members
		LinkerProcessor* linker;  // 08
	};
	static_assert(sizeof(LinkerConvertTypeFunctor) == 0x10);

	class VMTypeResolveFunctor : public ConvertTypeFunctor
	{
	public:
		inline static constexpr auto RTTI = RTTI_BSScript____VMTypeResolveFunctor;
		inline static constexpr auto VTABLE = VTABLE_BSScript____VMTypeResolveFunctor;
		~VMTypeResolveFunctor() override;  // 00

		bool ConvertVariableType(BSFixedString* a_typeAsString, TypeInfo& a_typeOut) override;  // 01
		// members
		RE::BSScript::Internal::VirtualMachine* vm;  // 08
	};
	static_assert(sizeof(VMTypeResolveFunctor) == 0x10);
}

namespace ModifyHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct PapyrusOpsPerFrameHook
	{
		struct PapyrusOpsModifier : Xbyak::CodeGenerator
		{
			PapyrusOpsModifier(std::uintptr_t beginLoop, std::uintptr_t endLoop)
			{
				if (REL::Module::IsAE()) {
					inc(r15d);
					cmp(r15d, Settings::GetSingleton()->VMtweaks.maxOpsPerFrame);
					jb("KeepLooping");
					mov(rcx, endLoop);
					jmp(rcx);
					L("KeepLooping");
					mov(rcx, beginLoop);
					jmp(rcx);
				} else {
					inc(r14d);
					cmp(r14d, Settings::GetSingleton()->VMtweaks.maxOpsPerFrame);
					mov(r8d, 10760);
					jb("KeepLooping");
					mov(rcx, endLoop);
					jmp(rcx);
					L("KeepLooping");
					mov(rcx, beginLoop);
					jmp(rcx);
				}
			}
		};

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0x498, 0xAB1, 0x498) };
			REL::Relocation<std::uintptr_t> beginLoop{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0xC0, 0xB0, 0xC0) };
			REL::Relocation<std::uintptr_t> endLoop{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0x4AB, 0xABE, 0x4AB) };

			auto newCompareCheck = PapyrusOpsModifier(beginLoop.address(), endLoop.address());
			int fillRange = REL::Module::IsAE() ? 0xD : 0x13;
			REL::safe_fill(target.address(), REL::NOP, fillRange);
			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(newCompareCheck.getSize());
			auto result = trampoline.allocate(newCompareCheck);
			auto& trampoline2 = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);

			logger::info("PapyrusOpsPerFrameHook hooked at address {:x}", target.address());
			logger::info("PapyrusOpsPerFrameHook hooked at offset {:x}", target.offset());
		}
	};

	struct StackDumpTimeoutHook
	{
		struct StackDumpTimeoutModifier : Xbyak::CodeGenerator
		{
			StackDumpTimeoutModifier(int timeoutMS)
			{
				// swap (add eax, 5000h) with this line
				add(eax, timeoutMS);
			}
		};

		// Install our hook at the specified address
		static inline void Install()
		{
			auto stackDumpTimeoutMS = Settings::GetSingleton()->VMtweaks.stackDumpTimeoutThreshold;
			if (stackDumpTimeoutMS < 0) {
				return;
			} else if (stackDumpTimeoutMS == 0) {
				installDisable();
			} else {
				installModifier(stackDumpTimeoutMS);
			}
		}

		static inline void installModifier(int timeoutMS)
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x6E, 0x71, 0x6E) };
			auto newTimeoutCheck = StackDumpTimeoutModifier(timeoutMS);
			assert(newTimeoutCheck.getSize() <= 0x5);
			REL::safe_fill(target.address(), REL::NOP, 0x5);  // Fill with NOP just in case
			REL::safe_write(target.address(), newTimeoutCheck.getCode(), newTimeoutCheck.getSize());

			logger::info("StackDumpTimeoutModifier hooked at address {:x}", target.address());
			logger::info("StackDumpTimeoutModifier hooked at offset {:x}", target.offset());
		}

		static inline void installDisable()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x6E, 0x71, 0x6E) };
			REL::safe_fill(target.address(), REL::NOP, 0x21);  // Disable the checks AND disable the stackdump flag
			logger::info("StackDumpTimeoutDisable hooked at address {:x}", target.address());
			logger::info("StackDumpTimeoutDisable hooked at offset {:x}", target.offset());
		}
	};

	struct FixToggleScriptsSaveHook
	{
		struct CallThunk : Xbyak::CodeGenerator
		{
			CallThunk(std::uintptr_t funct)
			{
				Xbyak::Label funcLabel;

				jmp(ptr[rip + funcLabel]);
				L(funcLabel);
				dq(funct);
			}
		};
		static void thunk(RE::SkyrimVM* a_this, bool a_frozen)
		{
			if (RE::Script::GetProcessScripts()) {  // Only unfreeze script processing if script processing is enabled
				a_this->frozenLock.Lock();
				a_this->isFrozen = a_frozen;
				a_this->frozenLock.Unlock();
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53205, 54016), REL::VariantOffset(0x46, 0x46, 0x41) };

			if (REL::Module::IsVR()) {
				stl::write_thunk_call<FixToggleScriptsSaveHook>(target.address());
			} else {
				// Save code uses jmp over call, and expects `SkyrimVM->SetFrozen()` to return from the save function
				// so we can't use a regular thunk call
				auto callThunk = CallThunk(reinterpret_cast<std::uintptr_t>(thunk));
				auto& trampoline = SKSE::GetTrampoline();
				SKSE::AllocTrampoline(callThunk.getSize());
				auto result = trampoline.allocate(callThunk);
				auto& trampoline2 = SKSE::GetTrampoline();
				SKSE::AllocTrampoline(14);
				trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);
			}

			logger::info("FixToggleScriptsSaveHook for saves hooked at address {:x}", target.address());
			logger::info("FixToggleScriptsSaveHook hooked at offset {:x}", target.offset());
		}
	};

	struct FixToggleScriptsDumpHook
	{
		static void thunk(RE::SkyrimVM* a_this, bool a_frozen)
		{
			if (RE::Script::GetProcessScripts()) {  // Only unfreeze script processing if script processing is enabled
				a_this->frozenLock.Lock();
				a_this->isFrozen = a_frozen;
				a_this->frozenLock.Unlock();
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53209, 54020), REL::VariantOffset(0xCC, 0xCC, 0xCC) };
			stl::write_thunk_call<FixToggleScriptsDumpHook>(target.address());

			logger::info("FixToggleScriptsDumpHook for stack dumps hooked at address {:x}", target.address());
			logger::info("FixToggleScriptsDumpHook hooked at offset {:x}", target.offset());
		}
	};

	struct FixScriptPageAllocation
	{
		// BSScript::SimpleAllocMemoryPagePolicy::GetLargestAvailablePage
		static RE::BSScript::IMemoryPagePolicy::AllocationStatus thunk(RE::BSScript::SimpleAllocMemoryPagePolicy* self, RE::BSTAutoPointer<RE::BSScript::MemoryPage>& a_newPage)
		{
			self->dataLock.Lock();
			int availablePageSize = self->maxAllocatedMemory - self->currentMemorySize;
			int currentMemorySizeTemp = self->currentMemorySize;
			if (availablePageSize < 0) {
				// set equal so the original function will return kOutOfMemory instead of unintentionally allocating a page
				self->currentMemorySize = self->maxAllocatedMemory;
			}
			RE::BSScript::IMemoryPagePolicy::AllocationStatus result = func(self, a_newPage);
			if (availablePageSize < 0) {
				// set back to original size
				self->currentMemorySize = currentMemorySizeTemp;
			}
			self->dataLock.Unlock();
			return result;
		}

		static inline std::uint32_t idx = 3;

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			stl::write_vfunc<RE::BSScript::SimpleAllocMemoryPagePolicy, FixScriptPageAllocation>();

			logger::info("FixScriptPageAllocation set!");
		}
	};

	struct FixIsHostileToActorCrash
	{
		// Easiest hook here is to replace the original IsHostileToActor callback with our own
		static std::uint64_t thunk(std::uint64_t unk, char* functionName, char* className, std::uintptr_t callback, VM** a_vm)
		{
			IsHostileToActor = callback; // Store the original so we can invoke it later
			return func(unk, functionName, className, reinterpret_cast<std::uintptr_t>(IsHostileToActorEx), a_vm);
		};

	    // Actor.IsHostileToActor(Actor akActor) crashes when akActor is NONE.
		// We will instead log an error and return false back, fixing the crash
		static bool IsHostileToActorEx(VM* a_vm, RE::VMStackID a_stackID, RE::Actor* a_self, RE::Actor* a_other) {
			if (!a_other) {
				a_vm->TraceStack(
					"Actor argument is NONE for Actor.IsHostileToActor()! Mod authors: This normally crashes the game in vanilla, but is fixed by Papyrus Tweaks NG",
					a_stackID,
					RE::BSScript::ErrorLogger::Severity::kError
				);
				return false;
			} else {
				return IsHostileToActor(a_vm, a_stackID, a_self, a_other);
			}
		}

		static inline REL::Relocation<decltype(IsHostileToActorEx)> IsHostileToActor;  // Original IsHostileToActor

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53960, 54784), REL::VariantOffset(0x2CA6, 0x2FDB, 0x2BA4) };
			stl::write_thunk_call<FixIsHostileToActorCrash>(target.address());

			logger::info("FixIsHostileToActorCrash hooked at address {:x}", target.address());
			logger::info("FixIsHostileToActorCrash hooked at offset {:x}", target.offset());
		}
	};

	struct FixDelayedTypeCast
	{
		auto static inline noneTypeString = RE::BSFixedString("NONE");
		// Note: This fix assumes that scripts are ALWAYS compiled properly (aka nothing is malformed), meaning the original function only fails
		// if the type doesn't exist (ex: Variable casted to SuperSecretClass, but SuperSecretClass doesn't exist)

		// BSScript::LinkerProcessor::ConvertVariableType
		static bool thunk(RE::BSScript::LinkerProcessor* self, RE::BSFixedString* a_name, RE::BSScript::TypeInfo& a_typeOut)
		{
			if (!func(self, a_name, a_typeOut)) { // If original call fails due to type not being present
				return func(self, &noneTypeString, a_typeOut); // Call it again, but to return a NONE type, as the script engine will treat it as such when running the script anyway
			}
			return true;
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98721, 105384), REL::VariantOffset(0x2C8, 0x30D, 0x2C8) };
			stl::write_thunk_call<FixDelayedTypeCast>(target.address());
			logger::info("FixDelayedTypeCast hooked at address {:x}", target.address());
			logger::info("FixDelayedTypeCast hooked at offset {:x}", target.offset());
			
			REL::Relocation<std::uintptr_t> target1{ RELOCATION_ID(98722, 105385), REL::VariantOffset(0x160, 0x1C7, 0x160) };
			stl::write_thunk_call<FixDelayedTypeCast>(target1.address());
			logger::info("FixDelayedTypeCast hooked at address {:x}", target1.address());
			logger::info("FixDelayedTypeCast hooked at offset {:x}", target1.offset());
		}
	};

	struct FixDelayedTypeCastVFunc
	{
		auto static inline noneTypeString = RE::BSFixedString("NONE");
		// See FixDelayedTypeCast for details, this is just the hook for the special vfunc version that jumps to the original
		static bool thunk(RE::BSScript::UnlinkedTypes::LinkerConvertTypeFunctor* self, RE::BSFixedString* a_name, RE::BSScript::TypeInfo& a_typeOut)
		{
			if (!func(self, a_name, a_typeOut)) {
				return func(self, &noneTypeString, a_typeOut);
			}
			return true;
		}

		static inline std::uint32_t idx = 1;

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			stl::write_vfunc<RE::BSScript::UnlinkedTypes::LinkerConvertTypeFunctor, FixDelayedTypeCastVFunc>();
			logger::info("FixDelayedTypeCastVFunc hook set!");
		}

	};

	struct EnableLoadDocStrings
	{
		// Hook the SkyrimVM's constructor that constructs CompiledScriptLoader, to enable doc string loading
		// This plays well with the load debug information hook
		static RE::BSScript::CompiledScriptLoader* thunk(RE::BSScript::CompiledScriptLoader* a_unmadeSelf, RE::SkyrimScript::Logger* a_logger, bool a_loadDebugInformation, bool a_loadDocStrings)
		{
			return func(a_unmadeSelf, a_logger, a_loadDebugInformation, true);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53108, 53919), REL::VariantOffset(0x604, 0x664, 0x604) };
			stl::write_thunk_call<EnableLoadDocStrings>(target.address());
			logger::info("EnableLoadDocStrings hooked at address {:x}", target.address());
			logger::info("EnableLoadDocStrings hooked at offset {:x}", target.offset());
		}
	};

	struct EnableLoadDebugInformation
	{
		// Hook the SkyrimVM's constructor that constructs CompiledScriptLoader, to enable debug information loading
		// This thunk hook plays well with the doc string hook
		static RE::BSScript::CompiledScriptLoader* thunk(RE::BSScript::CompiledScriptLoader* a_unmadeSelf, RE::SkyrimScript::Logger* a_logger, bool a_loadDebugInformation, bool a_loadDocStrings)
		{
			return func(a_unmadeSelf, a_logger, true, a_loadDocStrings);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53108, 53919), REL::VariantOffset(0x604, 0x664, 0x604) };
			stl::write_thunk_call<EnableLoadDebugInformation>(target.address());
			logger::info("EnableLoadDebugInformation hooked at address {:x}", target.address());
			logger::info("EnableLoadDebugInformation hooked at offset {:x}", target.offset());
		}
	};

	static inline void InstallHooks()
	{
		auto settings = Settings::GetSingleton();
		if (settings->VMtweaks.maxOpsPerFrame > 0) {
			PapyrusOpsPerFrameHook::Install();
		}
		if (settings->fixes.fixToggleScriptSave) {
			FixToggleScriptsSaveHook::Install();
			FixToggleScriptsDumpHook::Install();
		}
		if (settings->fixes.fixScriptPageAllocation) {
			FixScriptPageAllocation::Install();
		}
		if (settings->fixes.fixIsHostileToActorCrash) {
			FixIsHostileToActorCrash::Install();
		}
		if (settings->VMtweaks.stackDumpTimeoutThreshold > 0) {
			StackDumpTimeoutHook::Install();
		}
		if (settings->fixes.fixDelayedScriptBreakage) {
			FixDelayedTypeCast::Install();
			FixDelayedTypeCastVFunc::Install();
		}	
		if (settings->VMtweaks.enableDocStrings) {
			EnableLoadDocStrings::Install();
		}
		if (settings->VMtweaks.enableDebugInfo) {
			EnableLoadDebugInformation::Install();
		}
	}
}
