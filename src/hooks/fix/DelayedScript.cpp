#include "DelayedScript.h"

auto static inline noneTypeString = RE::BSFixedString("NONE");

// Note: This fix assumes that scripts are ALWAYS compiled properly (aka nothing is malformed), meaning the original function only fails
// if the type doesn't exist (ex: Variable casted to SuperSecretClass, but SuperSecretClass doesn't exist)
namespace hooks::fix::delayedscript
{
	// BSScript::LinkerProcessor::ConvertVariableType
	bool FixDelayedTypeCast::thunk(RE::BSScript::LinkerProcessor* self, RE::BSFixedString* a_name, RE::BSScript::TypeInfo& a_typeOut)
	{
		if (!func(self, a_name, a_typeOut)) { // If original call fails due to type not being present
			return func(self, &noneTypeString, a_typeOut); // Call it again, but to return a NONE type, as the script engine will treat it as such when running the script anyway
		}
		return true;
	}

	// VFunc version of the same logic
	bool FixDelayedTypeCastVFunc::thunk(RE::BSScript::UnlinkedTypes::LinkerConvertTypeFunctor* self, RE::BSFixedString* a_name, RE::BSScript::TypeInfo& a_typeOut)
	{
		if (!func(self, a_name, a_typeOut)) {
			return func(self, &noneTypeString, a_typeOut);
		}
		return true;
	}
}
