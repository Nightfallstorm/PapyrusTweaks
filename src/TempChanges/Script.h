#pragma once

#include "RE/C/CommandTable.h"
#include "RE/F/FormTypes.h"
#include "RE/T/TESForm.h"

// TEMPORARY HEADER DEFINITION UNTIL CLIB-NG UPDATES
namespace RETEMP
{
	enum class COMPILER_NAME
	{
		kDefaultCompiler,
		kSystemWindowCompiler,
		kDialogueCompiler
	};

	class ScriptCompiler
	{
	public:
	};
	static_assert(sizeof(ScriptCompiler) == 0x1);

	class Script
	{
	public:
		static bool GetProcessScripts()
		{
			using func_t = decltype(&GetProcessScripts);
			REL::Relocation<func_t> func{ RELOCATION_ID(21436, 21921) };
			return func();
		}

		static void SetProcessScripts(bool a_ProcessScripts)
		{
			using func_t = decltype(&SetProcessScripts);
			REL::Relocation<func_t> func{ RELOCATION_ID(21435, 21920) };
			return func(a_ProcessScripts);
		}

	private:
		void CompileAndRun_Impl(ScriptCompiler* a_compiler, COMPILER_NAME a_type, RE::TESObjectREFR* a_targetRef);
	};
}
