#include "DebugMode.h"

namespace hooks::tweak::debugmode
{
	// Hook the SkyrimVM's constructor that constructs CompiledScriptLoader, to enable doc string loading
	// This plays well with the load debug information hook
	RE::BSScript::CompiledScriptLoader* EnableLoadDocStrings::thunk(RE::BSScript::CompiledScriptLoader* a_unmadeSelf, RE::SkyrimScript::Logger* a_logger, bool a_loadDebugInformation, [[maybe_unused]] bool a_loadDocStrings)
	{
		return func(a_unmadeSelf, a_logger, a_loadDebugInformation, true);
	}


	// Hook the SkyrimVM's constructor that constructs CompiledScriptLoader, to enable debug information loading
	// This thunk hook plays well with the doc string hook
	RE::BSScript::CompiledScriptLoader* EnableLoadDebugInformation::thunk(RE::BSScript::CompiledScriptLoader* a_unmadeSelf, RE::SkyrimScript::Logger* a_logger, [[maybe_unused]] bool a_loadDebugInformation, bool a_loadDocStrings)
	{
		return func(a_unmadeSelf, a_logger, true, a_loadDocStrings);
	}
}