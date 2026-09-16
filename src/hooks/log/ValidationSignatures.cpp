#include "ValidationSignatures.h"
#undef GetObject

namespace
{
	// copied from stackoverflow. Just checks if string contained in other string no case sensitivity
	bool findStringIC(const std::string& strHaystack, const std::string& strNeedle)
	{
		auto it = std::ranges::search(strHaystack, strNeedle,
			[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); })
					  .begin();
		return (it != strHaystack.end());
	}

	std::string ConvertArgsToTypesAsString(RE::BSScrapArray<RE::BSScript::Variable>* a_varArray)
	{
		std::string types = "(";
		for (std::uint32_t i = 0; i < a_varArray->size(); i++) {
			if (auto& variable = a_varArray->data()[i]; variable.IsObject() && variable.GetObject() && variable.GetObject().get()) {
				types = types + variable.GetObject()->type->GetName();
			} else if (variable.IsObjectArray() && variable.GetArray() && variable.GetArray().get()) {
				types = types + variable.GetArray()->type_info().TypeAsString();
			} else {
				types = types + a_varArray->data()[i].GetType().TypeAsString();
			}
			if ((i + 1) < a_varArray->size()) {
				types = types + ",";
			}
		}
		types = types + ")";

		return types;
	}
	std::string ConvertFunctionToString(const RE::BSScript::IFunction* function)
	{
		std::string params;
		for (std::uint32_t i = 0; i < function->GetParamCount(); i++) {
			RE::BSFixedString name;
			RE::BSScript::TypeInfo type;
			function->GetParam(i, name, type);
			params = params + type.TypeAsString() + " " + name.c_str();
			if ((i + 1) < function->GetParamCount()) {
				params = params + ",";
			}
		}
		std::string result = std::format(
			"{}.{}({})",
			function->GetObjectTypeName().c_str(),
			function->GetName().c_str(), params);

		return result;
	}
}

namespace hooks::log::validationsignature
{
	std::uint64_t ValidationSignaturesHook::thunk(RE::BSScript::IFunction** a_function, RE::BSScrapArray<RE::BSScript::Variable>* a_varArray, char* a_outString, std::int32_t a_bufferSize)
	{
		const std::uint64_t result = func(a_function, a_varArray, a_outString, a_bufferSize);
		auto& function = *a_function;
		if (a_outString[0]) {
			// error occurred, let's improve it
			if (findStringIC(a_outString, "Type mismatch for argument")) {
				const std::string mismatchedFormat = "Function %s received incompatible arguments! Received types %s instead!";
				const std::string types = ConvertArgsToTypesAsString(a_varArray);
				snprintf(a_outString, a_bufferSize, mismatchedFormat.c_str(),
					ConvertFunctionToString(function).c_str(), types.c_str());
			} else if (findStringIC(a_outString, "Incorrect number of arguments passed")) {
				const std::string incorrectArgSizeFormat = "Incorrect number of arguments passed to function %s. Expected %u, got %u instead!. ";
				const std::string functionName = ConvertFunctionToString(*a_function);
				snprintf(a_outString, a_bufferSize, incorrectArgSizeFormat.c_str(), functionName.c_str(), function->GetParamCount(), a_varArray->size());
			} else if (findStringIC(a_outString, "Passing NONE to non-object argument")) {
				const std::string mismatchedFormat = "Function %s received NONE to non-object argument! Received types %s instead!";
				const std::string types = ConvertArgsToTypesAsString(a_varArray);
				snprintf(a_outString, a_bufferSize, mismatchedFormat.c_str(),
					ConvertFunctionToString(function).c_str(), types.c_str());
			} else {
				// Should never happen
			}
		}
		return result;
	}
}